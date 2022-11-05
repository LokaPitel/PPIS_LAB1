#include "KeyboardUtility.h"
#include "Model.h"
#include "View.h"
#include "Controller.h"

class SimulationApp
{
public:
	SimulationApp() {}

	static void runSimulation()
	{
		KeyboardUtility::init(100);

		Model model(20, 20);
		View view(&model);
		view.initMenues();

		Controller controller(&model, &view);

		while (true)
		{
			controller.nextStep();
		}
	}
};

int main()
{
	SimulationApp app;

	app.runSimulation();

	return 0;
}