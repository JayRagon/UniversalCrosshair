# UniversalCrosshair
A crosshair that will work for any windowed game

How to use:
1. Edit src code in main.cpp to draw the crosshair that you want, the line that draws by default is
   
ImGui::GetBackgroundDrawList()->AddCircleFilled({SWC, SHC}, 2.5f, ImColor(R, G, B), 0);

2. Compile and run
3. Set in-game window setting to windowed fullscreen or windowed
4. Disable in-game crosshair
