// Simple basecode showing how to create a window and attatch a vulkansurface
#define GATEWARE_ENABLE_CORE // All libraries need this
#define GATEWARE_ENABLE_SYSTEM // Graphics libs require system level libraries
#define GATEWARE_ENABLE_GRAPHICS // Enables all Graphics Libraries
// TODO: Part 3a
#define GATEWARE_ENABLE_MATH
#define GATEWARE_ENABLE_INPUT
#define GATEWARE_ENABLE_AUDIO
// Ignore some GRAPHICS libraries we aren't going to use
#define GATEWARE_DISABLE_GDIRECTX11SURFACE // we have another template for this
#define GATEWARE_DISABLE_GDIRECTX12SURFACE // we have another template for this
#define GATEWARE_DISABLE_GRASTERSURFACE // we have another template for this
#define GATEWARE_DISABLE_GOPENGLSURFACE // we have another template for this
// With what we want & what we don't defined we can include the API
#include "Gateware.h"
#include "renderer.h"
#include <fstream>
#include <iostream>
// open some namespaces to compact the code a bit
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;
// lets pop a window and use Vulkan to clear to a red screen
//GW::MATH::GMATRIXF Getdata(ifstream& f, GW::MATH::GMATRIXF& g);

//void Helper_Parse()
//{
//    std::ifstream game("../GameLevel.txt");
//    std::vector<std::string> names;
//    GW::MATH::GMATRIXF world[29];
//    int count = 0;
//
//    if (!game)
//    {
//        cerr << "Error: Error: Can't load file. Error" << std::endl;
//        exit(1);
//    }
//    if (game.is_open())
//    {
//        while (!game.eof())
//        {
//            char Buffer[128];
//
//            game.getline(Buffer, 128);
//
//            if (strcmp(Buffer, "MESH") == 0)
//            {
//                game.getline(Buffer, 128);
//                names.push_back(Buffer);
//                world[count] = Getdata(game, world[count]);
//                cerr << names[count] << endl;
//                count++;
//            }
//        }
//    }
//    game.close();
//}

//GW::MATH::GMATRIXF Getdata(ifstream& f, GW::MATH::GMATRIXF& g)
//{
//    char buff[128];
//
//    //row 1
//    f.getline(buff, 128, '(');
//    f.getline(buff, 128, ',');
//    g.row1.x = stof(buff); //x_pos
//    f.getline(buff, 128, ',');
//    g.row1.y = stof(buff); //y_pos
//    f.getline(buff, 128, ',');
//    g.row1.z = stof(buff); //z_pos
//    f.getline(buff, 128, ')');
//    g.row1.w = stof(buff);
//
//    //row 2
//    f.getline(buff, 128, '(');
//    f.getline(buff, 128, ',');
//    g.row2.x = stof(buff); //x_pos
//    f.getline(buff, 128, ',');
//    g.row2.y = stof(buff); //y_pos
//    f.getline(buff, 128, ',');
//    g.row2.z = stof(buff); //z_pos
//    f.getline(buff, 128, ')');
//    g.row2.w = stof(buff);
//
//    //row 3
//    f.getline(buff, 128, '(');
//    f.getline(buff, 128, ',');
//    g.row3.x = stof(buff); //x__pos
//    f.getline(buff, 128, ',');
//    g.row3.y = stof(buff); //y_pos
//    f.getline(buff, 128, ',');
//    g.row3.z = stof(buff); //z_pos
//    f.getline(buff, 128, ')');
//    g.row3.w = stof(buff);
//
//    //row 4
//    f.getline(buff, 128, '(');
//    f.getline(buff, 128, ',');
//    g.row4.x = stof(buff); //x_pos
//    f.getline(buff, 128, ',');
//    g.row4.y = stof(buff); //y_pos
//    f.getline(buff, 128, ',');
//    g.row4.z = stof(buff); //z_pos
//    f.getline(buff, 128, ')');
//    g.row4.w = stof(buff);
//    return g;
//}

int main()
{
	GWindow win;
	GEventResponder msgs;
	GVulkanSurface vulkan;
	//Helper_Parse();
	if (+win.Create(0, 0, 800, 600, GWindowStyle::WINDOWEDBORDERED))
	{
		// TODO: Part 1a
		win.SetWindowName("Simon Alzate - Level Renderer");
		VkClearValue clrAndDepth[2];
		clrAndDepth[0].color = { {1, 1, 1, 1} };
		clrAndDepth[1].depthStencil = { 1.0f, 0u };
		msgs.Create([&](const GW::GEvent& e) {
			GW::SYSTEM::GWindow::Events q;
			if (+e.Read(q) && q == GWindow::Events::RESIZE)
				clrAndDepth[0].color.float32[2] += 0.01f; // disable
			});
		win.Register(msgs);
#ifndef NDEBUG
		const char* debugLayers[] = {
			"VK_LAYER_KHRONOS_validation", // standard validation layer
			//"VK_LAYER_LUNARG_standard_validation", // add if not on MacOS
			//"VK_LAYER_RENDERDOC_Capture" // add this if you have installed RenderDoc
		};
		if (+vulkan.Create(	win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT, 
							sizeof(debugLayers)/sizeof(debugLayers[0]),
							debugLayers, 0, nullptr, 0, nullptr, false))
#else
		if (+vulkan.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
#endif
		{
			Renderer renderer(win, vulkan);
			while (+win.ProcessWindowEvents())
			{
				if (+vulkan.StartFrame(2, clrAndDepth))
				{
					renderer.UpdateCamera();
					renderer.Update();
					renderer.Render();
					vulkan.EndFrame(true);
				}
			}
		}
	}
	return 0; // that's all folks
}
