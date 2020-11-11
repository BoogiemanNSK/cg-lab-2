#include "anti_aliasing.h"

AntiAliasing::AntiAliasing(short width, short height) : AccelerationStructures(width, height)
{
}

AntiAliasing::~AntiAliasing()
{
}

void AntiAliasing::DrawScene()
{
	camera.SetRenderTargetSize(width * 2, height * 2);
	for (auto x = 0; x < width; x++) {
#pragma omp parallel for
		for (auto y = 0; y < height; y++) {
			Ray cameraRay1 = camera.GetCameraRay(x * 2, y * 2);
			Payload payload1 = TraceRay(cameraRay1, raytracing_depth);

			Ray cameraRay2 = camera.GetCameraRay(x * 2 + 1, y * 2);
			Payload payload2 = TraceRay(cameraRay2, raytracing_depth);

			Ray cameraRay3 = camera.GetCameraRay(x * 2, y * 2 + 1);
			Payload payload3 = TraceRay(cameraRay3, raytracing_depth);

			Ray cameraRay4 = camera.GetCameraRay(x * 2 + 1, y * 2 + 1);
			Payload payload4 = TraceRay(cameraRay4, raytracing_depth);
			
			float3 color = (payload1.color + payload2.color + payload3.color + payload4.color) / 4.0f;
			SetPixel(x, y, color);
		}
	}
}
