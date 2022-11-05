#pragma once
#include <sstream>
#include <stack>
#include "MathUtility.h"
#include "SetUtility.h"
#include "KeyboardUtility.h"
#include "Model.h"
#include "ViewState.h"
#include "View.h"
#include "EntityState.h"
#include "Entity.h"

class Controller
{
	Model* model;
	View* view;

	int heuristicFunctionForShortest(Position pos, Position target)
	{
		return Position::difference(pos, target);
	}

	int heuristicFunctionForSafest(Position pos)
	{
		return pos.getX() + pos.getY() + model->getDangerLevel(pos);
	}

public:
	Controller(Model* m, View* v) : model(m), view(v) {}

	void nextStep()
	{
		view->render();
		controlUponState();
	}

	void controlUponState()
	{
		if (view->getState() != CONSOLE)
			KeyboardUtility::handleKeyboard();
		view->nextState();
		view->render();

		ViewState previousViewState = view->getPreviousState();
		ViewState viewState = view->getState();

		if (viewState == OBSERVATION && previousViewState != CONTINUEMENU && previousViewState != STARTMENU)
		{
			nextStateOfModel();
		}

		else if (viewState == CONSOLE)
		{
			handleConsole();
		}
	}

	void actOfPlant(Entity* entity)
	{
		EntityState state = entity->getState();

		if (state == IDLE)
			;

		else if (state == REPRODUCING)
		{
			reproduceByPlant(entity);
		}
	}

	void actOfFood(Entity* entity)
	{
		EntityState state = entity->getState();

		if (state == IDLE)
			;
	}

	void actOfPlantEating(Entity* entity)
	{
		EntityState state = entity->getState();

		if (state == IDLE)
			randomlyWalk(entity);

		else if (state == WAITINGFORPAIR)
		{
			if (entity->hasCall())
			{
				entity->setTarget(entity->getCallee());
				entity->getTarget()->setTarget(entity);
				entity->setCallee(nullptr);
			}

			Entity* possiblePair;

			if (entity->isMale())
				possiblePair = model->getClosest(SetUtility::Intersection(
					SetUtility::Intersection(model->getAlive(), model->getFemale()), model->getPlantEating()), entity);

			else
				possiblePair = model->getClosest(SetUtility::Intersection(
					SetUtility::Intersection(model->getAlive(), model->getMale()), model->getPlantEating()), entity);

			if (possiblePair)
				entity->call(possiblePair);
		}

		else if (state == SEARCHINGFOREAT)
		{
			if (entity->getTarget() == nullptr || entity->getTarget()->getState() == DIED)
				entity->setTarget(model->getClosest(
					SetUtility::Intersection(
						SetUtility::Intersection(model->getAlive(), model->getEatable()),
						SetUtility::Union(model->getFood(), model->getPlants()))

					, entity));

			if (entity->getTarget())
				moveToTarget(entity, entity->getTarget());
		}

		else if (state == EATING)
			eatByPlantEating(entity, entity->getTarget());

		else if (state == RUNAWAY)
			moveToSafePlace(entity);

		else if (state == SEARCHINGFORPAIR)
		{
			if (entity->getTarget() == nullptr || entity->getTarget()->getState() == DIED)
				entity->setTarget(
					model->getClosest(
						SetUtility::Intersection(
							SetUtility::Intersection(
								SetUtility::Intersection(
									model->getAlive(), model->getReproducable()), model->getAnimals()), model->getPlantEating())

						, entity));

			moveToTarget(entity, entity->getTarget());
		}

		else if (state == REPRODUCING)
		{
			reproduceByPlantEating(entity, entity->getTarget());
		}
	}

	void actOfPredator(Entity* entity)
	{
		EntityState state = entity->getState();

		if (state == IDLE)
			randomlyWalk(entity);

		else if (state == WAITINGFORPAIR)
		{
			if (entity->hasCall())
			{
				entity->setTarget(entity->getCallee());
				entity->getTarget()->setTarget(entity);
				entity->setCallee(nullptr);
			}

			Entity* possiblePair;

			if (entity->isMale())
				possiblePair = model->getClosest(SetUtility::Intersection(
					SetUtility::Intersection(model->getAlive(), model->getFemale()), model->getPredators()), entity);

			else
				possiblePair = model->getClosest(SetUtility::Intersection(
					SetUtility::Intersection(model->getAlive(), model->getMale()), model->getPredators()), entity);

			if (possiblePair)
				entity->call(possiblePair);
		}

		else if (state == SEARCHINGFOREAT)
		{
			if (entity->getTarget() == nullptr || entity->getTarget()->getState() == DIED)
				entity->setTarget(model->getClosest(
					SetUtility::Intersection(
						SetUtility::Intersection(model->getAlive(), model->getEatable()), 
						SetUtility::Union(model->getFood(), model->getPlantEating()))

					, entity));

			if (entity->getTarget())
				moveToTarget(entity, entity->getTarget());
		}

		else if (state == EATING)
			eatByPredator(entity, entity->getTarget());

		else if (state == RUNAWAY)
			moveToSafePlace(entity);

		else if (state == SEARCHINGFORPAIR)
		{
			if (entity->getTarget() == nullptr || entity->getTarget()->getState() == DIED)
				entity->setTarget(
					model->getClosest(
						SetUtility::Intersection(
							SetUtility::Intersection(
								SetUtility::Intersection(
									model->getAlive(), model->getReproducable()), model->getAnimals()), model->getPredators())

						, entity));

			moveToTarget(entity, entity->getTarget());
		}

		else if (state == REPRODUCING)
		{
			reproduceByPredator(entity, entity->getTarget());
		}
	}

	void actUponState(Entity* entity)
	{
		if (entity->isPlantEating())
			actOfPlantEating(entity);

		else if (entity->isPredator())
			actOfPredator(entity);

		else if (entity->isPlant())
			actOfPlant(entity);

		else if (entity->isFood())
			actOfFood(entity);

		increaseHunger(entity, 1);
		increaseOld(entity, 1);
	}

	void makeActiveAllBorn()
	{
		std::set<Entity*> bornEntities = model->getInactive();

		for (Entity* entity : bornEntities)
			entity->makeActive();
	}

	void nextStateOf(Entity* entity)
	{
		if (entity->isAnimal())
			nextStateOfAnimal(entity);

		else if (entity->isPlant())
			nextStateOfPlant(entity);

		else if (entity->isFood())
			nextStateOfFood(entity);
	}

	void nextStateOfModel()
	{
		for (Entity* entity : model->getEntities())
		{
			if (!entity->isActive())
				continue;

			nextStateOf(entity);
			actUponState(entity);
		}

		handleAllDied();
		makeActiveAllBorn();
	}

	void nextStateOfAnimal(Entity* entity)
	{
		EntityState previousState = entity->getState();
		EntityState state = previousState;

		if (state == IDLE)
		{
			if (entity->isOld())
				state = DIED;

			else if (!entity->isPredator() && !model->isSafe(entity))
				state = RUNAWAY;

			else if (entity->isHunger() || entity->hasLowHealth())
				state = SEARCHINGFOREAT;

			else if (entity->isReproducable())
				state = WAITINGFORPAIR;
		}

		else if (state == SEARCHINGFOREAT)
		{
			if (entity->isOld() || entity->hasKillingHunger())
				state = DIED;

			else if (!entity->isPredator() && !model->isSafe(entity))
				state = RUNAWAY;

			else if (entity->getTarget() && model->isAdjacent(entity, entity->getTarget()))
				state = EATING;
		}

		else if (state == EATING)
		{
			if (entity->isOld())
				state = DIED;

			else if (!entity->isPredator() && !model->isSafe(entity))
				state = RUNAWAY;

			else if (entity->hasAdmissibleHunger())
				state = IDLE;

			else if (!entity->getTarget() || !model->isAdjacent(entity, entity->getTarget()))
				state = SEARCHINGFOREAT;
		}

		else if (state == RUNAWAY)
		{
			if (entity->isOld() || entity->hasZeroHealth() || entity->hasKillingHunger())
				state = DIED;

			else if (model->isSafe(entity))
				state = IDLE;
		}

		else if (state == WAITINGFORPAIR)
		{
			if (!entity->isPredator() && !model->isSafe(entity))
				state = RUNAWAY;

			else if (entity->isHunger())
				state = SEARCHINGFOREAT;

			else if (!entity->isReproducable())
				state == IDLE;

			else if (entity->hasPair())
				state = SEARCHINGFORPAIR;
		}

		else if (state == SEARCHINGFORPAIR)
		{
			if (!entity->isPredator() && !model->isSafe(entity))
				state = RUNAWAY;

			else if (entity->isHunger())
				state = SEARCHINGFOREAT;

			else if (!entity->isReproducable())
				state = IDLE;

			else if (!entity->hasPair())
				state = WAITINGFORPAIR;

			else if (model->isAdjacent(entity, entity->getTarget()))
				state = REPRODUCING;
		}

		else if (state == REPRODUCING)
		{
			if (!entity->isPredator() && !model->isSafe(entity))
				state = RUNAWAY;

			else if (entity->isHunger())
				state = SEARCHINGFOREAT;

			else if (!entity->isReproducable())
				state = IDLE;

			else if (!entity->hasPair())
				if (entity->isMale())
					state = WAITINGFORPAIR;
		}

		if (previousState == REPRODUCING && previousState != state
			|| previousState == SEARCHINGFORPAIR && previousState != state && state != REPRODUCING
			|| previousState == WAITINGFORPAIR && previousState != state && state != SEARCHINGFORPAIR)
		{
			entity->byeTarget();
		}

		if (state == DIED)
		{
			for (Entity* entity : model->getEntities())
			{
				if (entity->getTarget() == entity)
					entity->setTarget(nullptr);
			}
		}

		entity->setState(state);
	}

	void nextStateOfPlant(Entity* entity)
	{
		EntityState previousState = entity->getState();
		EntityState state = previousState;

		if (state == IDLE)
		{
			if (entity->hasZeroHealth() || entity->isOld())
				state = DIED;

			else if (entity->isReproducable())
				state = REPRODUCING;
		}

		else if (state == REPRODUCING)
		{
			if (entity->hasZeroHealth())
				state = DIED;

			else if (!entity->isReproducable())
				state = IDLE;
		}

		if (previousState == REPRODUCING && previousState != state
			|| previousState == SEARCHINGFORPAIR && previousState != state && state != REPRODUCING)
			entity->byeTarget();

		if (state == DIED)
		{
			for (Entity* entity : model->getEntities())
			{
				if (entity->getTarget() == entity)
					entity->setTarget(nullptr);
			}
		}

		entity->setState(state);
	}

	void nextStateOfFood(Entity* entity)
	{
		EntityState previousState = entity->getState();
		EntityState state = previousState;

		if (state == IDLE)
		{
			if (entity->hasZeroHealth() || entity->isOld())
				state = DIED;
		}

		if (state == DIED)
		{
			for (Entity* entity : model->getEntities())
			{
				if (entity->getTarget() == entity)
					entity->setTarget(nullptr);
			}
		}

		entity->setState(state);
	}

	void handleAllDied()
	{
		Entity* toDelete = nullptr;
		for (Entity* entity : model->getEntities())
		{
			if (toDelete)
			{
				for (Entity* enity : model->getEntities())
				{
					/*if (entity->getTarget() == toDelete)
						entity->setTarget(nullptr);

					if (entity->getCallee() == toDelete)
						entity->setCallee(nullptr);*/

					handleDied(toDelete);
				}

				model->getEntities().erase(toDelete);
				toDelete = nullptr;
			}

			if (entity->getState() == DIED)
				toDelete = entity;
		}

		if (toDelete)
		{
			handleDied(toDelete);
			model->getEntities().erase(toDelete);
		}
	}

	void handleDied(Entity* entity)
	{
		for (Entity* another : model->getEntities())
		{
			if (another->getTarget() == entity)
				another->setTarget(nullptr);

			if (another->getCallee() == entity)
				another->setCallee(nullptr);
		}
	}

	void handleConsole()
	{
		std::string command;
		std::getline(std::cin, command);

		if (command.empty() || command == "\n")
			return;

		std::stringstream commandTokens = std::stringstream(command);
		std::vector<std::string> tokens;

		std::string token;
		while (std::getline(commandTokens, token, ' '))
			tokens.push_back(token);

		command = tokens[0];

		int argumentCount = tokens.size() - 1;

		if (command == "observe")
		{
			if (argumentCount != 0)
				return;

			view->setObserveCommand();
		}

		else if (command == "info")
		{
			if (argumentCount != 0)
				return;

			view->setInfoCommand();
			//view->setCommandLine("control");
		}

		else if (command == "addplanteatingmale")
		{
			if (argumentCount != 2)
				return;

			model->addPlantEatingMale(Position(std::stoi(tokens[1]), std::stoi(tokens[2])));
		}

		else if (command == "addplanteatingfemale")
		{
			if (argumentCount != 2)
				return;

			model->addPlantEatingFemale(Position(std::stoi(tokens[1]), std::stoi(tokens[2])));
		}

		else if (command == "addpredatormale")
		{
			if (argumentCount != 2)
				return;

			model->addPredatorMale(Position(std::stoi(tokens[1]), std::stoi(tokens[2])));
		}

		else if (command == "addpredatorfemale")
		{
			if (argumentCount != 2)
				return;

			model->addPredatorFemale(Position(std::stoi(tokens[1]), std::stoi(tokens[2])));
		}

		else if (command == "addplant")
		{
			if (argumentCount != 2)
				return;

			model->addPlant(Position(std::stoi(tokens[1]), std::stoi(tokens[2])));
		}

		else if (command == "addfood")
		{
			if (argumentCount != 2)
				return;

			model->addFood(Position(std::stoi(tokens[1]), std::stoi(tokens[2])));
		}

		else if (command == "delete")
		{
			if (argumentCount != 2)
				return;

			Entity* ent = model->getIn(Position(std::stoi(tokens[1]), std::stoi(tokens[2])));

			handleDied(ent);

			model->getEntities().erase(ent);
		}
	}

	void increaseHealth(Entity* eating, int healthAddition)
	{
		int health = eating->getHealth();

		health += healthAddition;

		if (health > eating->getMaxHealth())
			health = eating->getMaxHealth();

		eating->setHealth(health);
	}

	void decreaseHealth(Entity* eating, int healthDecreasion)
	{
		int health = eating->getHealth();

		health -= healthDecreasion;

		if (health < 0)
			health = 0;

		eating->setHealth(health);
	}

	void increaseHunger(Entity* eating, int hungerDecreasion)
	{
		int hunger = eating->getHunger();

		hunger += hungerDecreasion;

		if (hunger > eating->getMaxHunger())
			hunger = eating->getMaxHunger();

		eating->setHunger(hunger);
	}

	void decreaseHunger(Entity* eating, int hungerDecreasion)
	{
		int hunger = eating->getHunger();

		hunger -= hungerDecreasion;

		if (hunger < 0)
			hunger = 0;

		eating->setHunger(hunger);
	}

	void increaseOld(Entity* eating, int oldIncreasion)
	{
		int old = eating->getOld();

		old += oldIncreasion;

		if (old > eating->getMaxOld())
			old = eating->getMaxOld();

		eating->setOld(old);
	}

	void randomlyWalk(Entity* entity)
	{
		std::set<Position> freeAdjacentPositions = model->getFreeAdjacent(entity->getPosition());

		if (!freeAdjacentPositions.empty())
		{
			Position pos = SetUtility::randomFrom(freeAdjacentPositions);

			entity->setPosition(pos);
			//moveTo(entity, pos);
		}
	}

	void moveToTarget(Entity* entity, Entity* target)
	{
		std::set<Position> freeAdjacent = model->getFreeAdjacent(entity->getPosition());


		if (!freeAdjacent.empty())
		{
			Position min = *freeAdjacent.begin();

			for (Position pos : freeAdjacent)
				if (heuristicFunctionForShortest(pos, target->getPosition()) < heuristicFunctionForShortest(min, target->getPosition()))
					min = pos;

			entity->setPosition(min);
		}
	}

	void moveToSafePlace(Entity* entity)
	{
		Position pos = model->getClosest(model->getSafePlaces(), entity->getPosition());

		if (pos != Position(-1, -1))
		{
			std::set<Position> freeAdjacent = model->getFreeAdjacent(entity->getPosition());

			if (!freeAdjacent.empty())
			{
				Position min = *freeAdjacent.begin();

				for (Position position : freeAdjacent)
					if (heuristicFunctionForShortest(position, pos) < heuristicFunctionForShortest(min, pos))
						min = position;

				entity->setPosition(min);
			}
		}
	}

	void eatByPlantEating(Entity* eating, Entity* eatable)
	{
		int healthAddition = 0;
		int hungerDecreasion = 0;

		if (eatable->isPlant())
		{
			healthAddition = 5;
			hungerDecreasion = 10;
		}

		else if (eatable->isFood())
		{
			healthAddition = 2;
			hungerDecreasion = 4;
		}

		increaseHealth(eating, healthAddition);
		decreaseHunger(eating, hungerDecreasion);

		decreaseHealth(eatable, healthAddition);
		nextStateOf(eatable);
	}

	void reproduceByPlantEating(Entity* one, Entity* another)
	{
		std::set<Position> freePositions = model->getFreeAdjacent(one->getPosition());

		if (freePositions.empty())
			return;

		int randomNumber = MathUtility::randomInt(1, 4);

		if (randomNumber == 1)
		{
			Position pos = SetUtility::randomFrom(freePositions);

			int whetherMale = MathUtility::randomInt(0, 1);

			if (whetherMale)
				model->bornNewPlantEatingMale(pos);

			else
				model->bornNewPlantEatingFemale(pos);
		}
	}

	void eatByPredator(Entity* eating, Entity* eatable)
	{
		int healthAddition = 0;
		int hungerDecreasion = 0;

		if (eatable->isPlantEating())
		{
			healthAddition = 6;
			hungerDecreasion = 5;
		}

		else if (eatable->isFood())
		{
			healthAddition = -2;
			hungerDecreasion = 4;
		}

		increaseHealth(eating, healthAddition);
		decreaseHunger(eating, hungerDecreasion);

		decreaseHealth(eatable, healthAddition);
		nextStateOf(eatable);
	}

	void reproduceByPredator(Entity* one, Entity* another)
	{
		std::set<Position> freePositions = model->getFreeAdjacent(one->getPosition());

		if (freePositions.empty())
			return;

		Position pos = SetUtility::randomFrom(freePositions);

		int randomNumber = MathUtility::randomInt(1, 8);

		if (randomNumber == 1)
		{
			int whetherMale = MathUtility::randomInt(0, 1);

			if (whetherMale)
				model->bornNewPredatorMale(pos);

			else
				model->bornNewPredatorFemale(pos);
		}
	}

	void reproduceByPlant(Entity* one)
	{
		std::set<Position> freePositions = model->getFreeAdjacent(one->getPosition());

		if (freePositions.empty())
			return;

		Position pos = SetUtility::randomFrom(freePositions);

		int randomNumber = MathUtility::randomInt(1, 8);

		if (randomNumber == 1)
		{
			model->bornNewPlant(pos);
		}
	}
};