#pragma once
#include "kernelinterface.hpp"

// inject mouse movement, the driver needs to be loaded and i am keeping it private thank you very much

int Move(LONG x, LONG y)
{
	KernelInterface Driver = KernelInterface("\\\\.\\ragedrv1");

	return Driver.SetMouse(x, y, 0);
}

void Click()
{
	KernelInterface Driver = KernelInterface("\\\\.\\ragedrv1");
	Driver.SetMouse(0, 0, 1);
	Sleep(10);
	Driver.SetMouse(0, 0, 2);
	Sleep(10);
}