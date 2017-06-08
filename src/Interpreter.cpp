#include "Interpreter.h"
#include <sstream>
#include <codecvt>
#include "WindowsUtilities.h"
#include "Logger.h"

Interpreter::Interpreter(VBoxMainController* vbmc)
{
	this->vbmc = vbmc;
}


Interpreter::~Interpreter()
{
}

std::wstring Interpreter::search()
{
	auto toSearch(popInterpretQueue());
	vmSelected = vbmc->getVMByNameOrUUID(toSearch);
	if (!vmSelected)
		return error(L"NoVmNameMatched");
	std::wstring returnString(L"SUCCEED VmName ");
	returnString += vmSelected->getName();
	return returnString;
}

std::wstring Interpreter::control()
{
	if (!vmSelected)
		return error(L"NoVmSelected");
	auto cmd(popInterpretQueue());
	if (cmd == L"start")
	{
		vmSelected->launch();
		return L"SUCCEED";
	}
	if (cmd == L"stop")
	{
		vmSelected->powerButton();
		return L"SUCCEED";
	}
	if (cmd == L"output")
	{
		vmSelected->exportOVA(popInterpretQueue());
		return L"SUCCEED";
	}
	return error(L"UnknownCommand");
}

std::wstring Interpreter::get()
{
	auto cmd(popInterpretQueue());

	if (cmd == L"backendVersion")
	{
		return vbmc->getVBoxVersion();
	}
	if (cmd == L"backend")
	{
		return L"VirtualBox Windows";
	}
	if (cmd == L"importStat")
	{
		if (vbmc->appliance()->isBusy())
		{
			return L"BUSY";
		}
		return L"IDLE";
	}
	if (cmd == L"importProgress")
	{
		return std::to_wstring(vbmc->appliance()->getProgress());
	}
	if (cmd == L"importedNewName")
	{
		return vbmc->appliance()->getNewVMName();
	}
	if (cmd == L"importSucceeded")
	{
		if (vbmc->appliance()->isSucceeded())
		{
			return L"true";
		}
		return L"false";
	}
	if (cmd == L"ipconfigAndSave")
	{
		WindowsUtilities::ipconfigAndSave();
		return L"done";
	}
	if (cmd == L"userHomePath")
	{
		return WindowsUtilities::getHomePathWstring();
	}
	if (cmd == L"systemTotalRAM")
	{
		return WindowsUtilities::getRAMSize();
	}
	if (cmd == L"vmxSupport")
	{
		return WindowsUtilities::getVMXSupport();
	}
	if (cmd == L"processorCount")
	{
		return WindowsUtilities::getCPUCount();
	}
	if (vmSelected)
	{
		if (cmd == L"machineState")
		{
			return vmSelected->getMachineStatus();
		}
		if (cmd == L"machineId")
		{
			return vmSelected->getUuid();
		}
		if (cmd == L"machineName")
		{
			return vmSelected->getName();
		}

	}
	return error(L"UnknownCommand");
}

std::wstring Interpreter::error(std::wstring what)
{
	std::wstring string(L"ERROR ");
	string += what;
	return string;
}

std::wstring Interpreter::import()
{
	std::wstringstream wstringstream;
	while (!InterpretQueue->empty())
	{
		wstringstream << popInterpretQueue() << " ";
	}
	//substr because of the last space " "
	vbmc->appliance()->import(wstringstream.str().substr(0, wstringstream.str().size() - 1));
	if (vbmc->appliance()->isBusy())
	{
		return(L"SUCCESS");
	}
	return(L"ERROR UnImplemented error code");
}

std::wstring Interpreter::set()
{
	if (!vmSelected)
	{
		return L"ERROR";
	}
	auto cmd(popInterpretQueue());
	if (cmd == L"machineName")
	{
		std::wstringstream wstringstream;
		while (!InterpretQueue->empty())
		{
			wstringstream << popInterpretQueue() << " ";
		}
		vmSelected->setName(wstringstream.str().substr(0, wstringstream.str().size() - 1));
		return L"SUCCEED";
		//TODO what will happen if it failed?
	}
	if (cmd == L"cpuCount")
	{
		auto numberString(popInterpretQueue());
		try
		{
			vmSelected->setCPUCount(std::stoi(numberString));
		}
		catch (...)
		{
			vmSelected->setCPUCount(1);
		}
		//TODO fail
		return L"SUCCEED";
	}
	if (cmd == L"ramSize")
	{
		auto numberString(popInterpretQueue());
		try
		{
			vmSelected->setRAMSize(std::stoi(numberString));
		}
		catch (...)
		{
			vmSelected->setRAMSize(512);
		}
		//TODO fail
		return L"SUCCEED";
	}
	if (cmd == L"addPortForwarding")
	{
		auto guestPort(std::stoi(popInterpretQueue()));
		auto hostPort(std::stoi(popInterpretQueue()));
		try
		{
			if (vmSelected->getMachineStatus() == L"Running")
			{
				vmSelected->addNATPortForwardingRuleOnline(static_cast<unsigned int>(guestPort), static_cast<unsigned int>(hostPort));
			}
			else
			{
				vmSelected->addNATPortForwardingRuleOffline(static_cast<unsigned int>(guestPort), static_cast<unsigned int>(hostPort));
			}
		}
		catch (...)
		{
			Logger::log("Interpreter", __func__, InfoLevel::ERR, "can not add port forwarding rule");
		}
		//TODO fail
		return L"SUCCEED";
	}
	if (cmd == L"removePortForwarding")
	{
		auto guestPort(std::stoi(popInterpretQueue()));
		try
		{
			if(vmSelected->getMachineStatus() == L"Running")
			{
				vmSelected->removeNATPortForwardingRuleOnline(static_cast<unsigned int>(guestPort));
			}
			else
			{
				vmSelected->removeNATPortForwardingRuleOffline(static_cast<unsigned int>(guestPort));
			}
		}
		catch(...)
		{
			Logger::log("Interpreter", __func__, InfoLevel::ERR, "can not remove port forwarding rule");
		}
		//TODO fail
		return L"SUCCEED";
	}
	return L"ERROR";
}

std::wstring Interpreter::del()
{
	auto cmd(popInterpretQueue());
	try
	{
		VirtualMachine::deleteMachineFully(cmd);
	}
	catch (std::exception& e)
	{
		return error(WindowsUtilities::toWstring(e.what()));
	}
	VBoxMainController::refreshCache();
	return L"SUCCEED";
}

std::wstring Interpreter::interpret()
{
	//TODO refactor using Design Pattern
	auto cmd(popInterpretQueue());
	if (cmd == L"select")
	{
		if (popInterpretQueue() == L"search")
		{
			return search();
		}
	}
	if (cmd == L"control")
	{
		return control();
	}
	if (cmd == L"get")
	{
		return get();
	}
	if (cmd == L"set")
	{
		return set();
	}
	if (cmd == L"import")
	{
		return import();
	}
	if (cmd == L"del")
	{
		return del();
	}
	if (cmd == L"exit")
	{
		return L"exit";
	}
	if (cmd == L"keepAlive")
	{
		return L"keepAlive";
	}
	return error(L"UnknownCommand");
}

std::wstring Interpreter::popInterpretQueue() const
{
	if (InterpretQueue->empty())
	{
		return std::wstring();
	}
	auto front(InterpretQueue->front());
	InterpretQueue->pop();
	return front;
}

std::wstring Interpreter::run(std::wstring wstring)
{
	if (InterpretQueue)
		delete InterpretQueue;
	InterpretQueue = makeCommandQueue(wstring);
	try
	{
		return interpret();
	}
	catch (std::exception& e)
	{
		return error(WindowsUtilities::toWstring(e.what()));
	}
}

void Interpreter::reset()
{
	vmSelected = nullptr;
}

std::queue<std::wstring>* Interpreter::makeCommandQueue(const std::wstring& string)
{
	std::wstringstream stringstream(string);
	std::wstring buffer;
	auto stringQueue = new std::queue<std::wstring>;
	while (stringstream >> buffer)
	{
		stringQueue->push(buffer);
	}
	return stringQueue;
}
