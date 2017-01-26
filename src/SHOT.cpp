#include "SHOTSolver.h"

SHOTSolver *solver = NULL;
FileUtil *fileUtil = NULL;

std::string startmessage;

int main(int argc, char *argv[])
{
	ProcessInfo *processInfo;
	processInfo = ProcessInfo::getInstance();
	processInfo->startTimer("Total");

	// Adds a file output
	osoutput->AddChannel("shotlogfile");

	/*osoutput->OSPrint(ENUM_OUTPUT_AREA_main, ENUM_OUTPUT_LEVEL_always, "Always\n");
	 osoutput->OSPrint(ENUM_OUTPUT_AREA_main, ENUM_OUTPUT_LEVEL_warning, "Warning\n");
	 osoutput->OSPrint(ENUM_OUTPUT_AREA_main, ENUM_OUTPUT_LEVEL_debug, "Debug\n");
	 osoutput->OSPrint(ENUM_OUTPUT_AREA_main, ENUM_OUTPUT_LEVEL_detailed_trace, "Detailed trace\n");
	 osoutput->OSPrint(ENUM_OUTPUT_AREA_main, ENUM_OUTPUT_LEVEL_error, "Error\n");
	 osoutput->OSPrint(ENUM_OUTPUT_AREA_main, ENUM_OUTPUT_LEVEL_info, "Info\n");
	 osoutput->OSPrint(ENUM_OUTPUT_AREA_main, ENUM_OUTPUT_LEVEL_summary, "Summary\n");*/
	//osoutput->OSPrint(ENUM_OUTPUT_AREA_main, ENUM_OUTPUT_LEVEL_trace, "Trace\n");
	startmessage = ""
			"┌─────────────────────────────────────────────────────────────────────┐\n"
			"│          SHOT - Supporting Hyperplane Optimization Toolkit          │\n"
			"├─────────────────────────────────────────────────────────────────────┤\n"
			"│ - Implementation by Andreas Lundell (andreas.lundell@abo.fi)        │\n"
			"│ - Based on the Extended Supporting Hyperplane (ESH) algorithm       │\n"
			"│   by Jan Kronqvist, Andreas Lundell and Tapio Westerlund            │\n"
			"│   Åbo Akademi University, Turku, Finland                            │\n"
			"└─────────────────────────────────────────────────────────────────────┘\n";

	if (argc == 1)
	{
		std::cout << startmessage << std::endl;
		std::cout << "Usage: filename.osil options.osol results.osrl trace.trc" << std::endl;

		delete processInfo;

		return (0);
	}

	boost::filesystem::path resultFile, optionsFile, traceFile;

	fileUtil = new FileUtil();
	solver = new SHOTSolver();

	if (argc == 2)
	{
		optionsFile = boost::filesystem::path(boost::filesystem::current_path() / "options.xml");

		if (!boost::filesystem::exists(optionsFile))
		{
			fileUtil->writeFileFromString(optionsFile.string(), solver->getOSol());
		}

		resultFile = boost::filesystem::path(boost::filesystem::current_path() / "results.osrl");
		traceFile = boost::filesystem::path(boost::filesystem::current_path() / "trace.trc");
	}
	else if (argc == 3)
	{
		if (!boost::filesystem::exists(argv[2]))
		{
			std::cout << startmessage << std::endl;
			std::cout << "Options file not found!" << std::endl;

			delete fileUtil, solver, processInfo;

			return (0);
		}

		optionsFile = boost::filesystem::path(argv[2]);
		resultFile = boost::filesystem::path(boost::filesystem::current_path() / "results.osrl");
		traceFile = boost::filesystem::path(boost::filesystem::current_path() / "trace.trc");
	}
	else if (argc == 4)
	{
		if (!boost::filesystem::exists(argv[2]))
		{
			std::cout << startmessage << std::endl;
			std::cout << "Options file not found!" << std::endl;

			delete fileUtil, solver, processInfo;

			return (0);
		}

		optionsFile = boost::filesystem::path(argv[2]);
		resultFile = boost::filesystem::path(argv[3]);
		traceFile = boost::filesystem::path(boost::filesystem::current_path() / "trace.trc");
	}
	else
	{
		if (!boost::filesystem::exists(argv[2]))
		{
			std::cout << startmessage << std::endl;
			std::cout << "Options file not found!" << std::endl;

			delete fileUtil, solver, processInfo;

			return (0);
		}

		optionsFile = boost::filesystem::path(argv[2]);
		resultFile = boost::filesystem::path(argv[3]);
		traceFile = boost::filesystem::path(argv[4]);
	}

	try
	{
		if (!boost::filesystem::exists(argv[1]))
		{
			std::cout << startmessage << std::endl;
			std::cout << "Problem file not found!" << std::endl;

			delete fileUtil, solver, processInfo;

			return (0);
		}

		std::string osilFileName = argv[1];

		if (!solver->setOptions(optionsFile.string()))
		{
			delete fileUtil, solver, processInfo;

			std::cout << startmessage << std::endl;
			std::cout << "Cannot set options!" << std::endl;
			return (0);
		}

		// Prints out the welcome message to the logging facility
		processInfo->outputSummary(startmessage);

		if (!solver->setProblem(osilFileName))
		{
			processInfo->outputError("Error when reading problem file.");

			delete fileUtil, solver, processInfo;

			return (0);
		}

		if (!solver->solveProblem()) // solve problem
		{
			processInfo->outputError("Error when solving problem.");

			delete fileUtil, solver, processInfo;

			return (0);
		}
	}
	catch (const ErrorClass& eclass)
	{
		processInfo->outputError(eclass.errormsg);
		delete fileUtil, solver, processInfo;

		return (0);
	}

	processInfo->stopTimer("Total");

	std::string osrl = solver->getOSrl();

	fileUtil->writeFileFromString(resultFile.string(), osrl);

	std::string trace = solver->getTraceResult();
	fileUtil->writeFileFromString(traceFile.string(), trace);

	processInfo->outputSummary("\n"
			"┌─── Solution time ──────────────────────────────────────────────────────────────┐");

	for (auto T : processInfo->timers)
	{
		auto elapsed = T.elapsed();

		if (elapsed > 0)
		{
			auto tmpLine = boost::format("%1%: %|54t|%2%") % T.description % elapsed;

			processInfo->outputSummary("│ " + tmpLine.str());
		}
	}

	processInfo->outputSummary("└────────────────────────────────────────────────────────────────────────────────┘");

	delete fileUtil, solver, processInfo;
	return (0);
}
