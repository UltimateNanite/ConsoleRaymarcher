#include <iostream>

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "camera.h"
#include <chrono>


// Console Raymarcher - By Chloe Taylor
// Released under the IDC license. Do whatever you want with this, please credit me if you use code from this, cheers
// 
// camera.h by Joey de Vries, adapted from their OpenGL tutorial series https://learnopengl.com/Getting-started/Camera
//
// Just a little software raymarcher I wrote for fun in two days, not optimized or good at all but it works and I'm happy with it.
// I might update it with lighting and stuff?? Probably not though
// Follow me on Twitter @trans_disaster if you like gay shit
//
// Open source is the socialism of software you can't change my mind
// And yes that's a good thing


#ifdef __unix__
#define OS_Linux

#elif defined(_WIN32) || defined(WIN32)
#define OS_Windows
#endif

#ifdef OS_Windows
#include <windows.h>
#endif

// Portable function that returns the console size in columns and rows. Haven't tested Unix version, sorry
glm::uvec2 getConsoleSizeCR() {
#ifdef OS_Windows
	
	CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	return glm::uvec2(csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
	
#elif defined(OS_Linux)

	struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	return {w.ws_col, w.ws_row};
#endif
}



glm::uvec2 consoleSize = getConsoleSizeCR();
std::vector<wchar_t> screen;

void setChar(glm::uvec2 pos, wchar_t c) {
	screen[pos.y * consoleSize.x + pos.x] = c;
}

wchar_t getChar(glm::uvec2 pos) {
	return screen[pos.y * consoleSize.x + pos.x];
}

void writeString(glm::uvec2 pos, const std::string& str) {
	for (size_t i = 0; i < str.size(); i++) {
		screen[pos.y * consoleSize.x + pos.x + i] = str[i];
	}
}

template<typename T>
float length(const T& t) { return std::sqrt(t.x * t.x + t.y * t.y + t.z * t.z); }

namespace rm {
	class Position {
	protected:
		glm::vec3 m_pos;
		explicit Position(glm::vec3 pos) : m_pos(pos) {}
	public:
		glm::vec3 getPosition() const { return m_pos; }
		void setPosition(glm::vec3 position) { m_pos = position;}
	};
	
	class Object {
	public:
		virtual ~Object() = default;
		virtual float signedDistance(glm::vec3 ray) const = 0;
	};

	class Sphere: public Object, public Position {
		float m_radius;
	public:
		float signedDistance(glm::vec3 ray) const override {
			return length(m_pos - ray) - m_radius;
		};
		
		Sphere(glm::vec3 pos, float radius) : Position(pos), m_radius(radius) {}

		float getRadius() const { return m_radius; }
		void setRadius(float radius) { m_radius = radius; }
	};
}

std::vector<std::unique_ptr<rm::Object>> objects;

int main() {
	screen.resize(consoleSize.x * consoleSize.y);
	
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	Camera cam;
	cam.Position = glm::vec3(0.f);

	objects.push_back(std::make_unique<rm::Sphere>(glm::vec3(0.f,0.f,-10.f), 1.f));
	
	float delta = .016f;

	POINT lastMousePos;
	GetCursorPos(&lastMousePos);
	
	while (true) {
		auto start = std::chrono::system_clock::now();
		//Update screen size
		glm::uvec2 newConsoleSize = getConsoleSizeCR();
		if (newConsoleSize != consoleSize) {
			screen.resize(newConsoleSize.x * newConsoleSize.y);
			consoleSize = newConsoleSize;
		}


		//Update camera
		if (GetKeyState('A') & 0x8000)
			cam.ProcessKeyboard(LEFT, delta);
		if (GetKeyState('D') & 0x8000)
			cam.ProcessKeyboard(RIGHT, delta);
		if (GetKeyState('W') & 0x8000)
			cam.ProcessKeyboard(FORWARD, delta);
		if (GetKeyState('S') & 0x8000)
			cam.ProcessKeyboard(BACKWARD, delta);

		POINT mousePos;
		GetCursorPos(&mousePos);
		cam.ProcessMouseMovement(mousePos.x - lastMousePos.x, mousePos.y - lastMousePos.y);
		lastMousePos = mousePos;
		//Clear screen
		std::fill(screen.begin(), screen.end(), ' ');

		unsigned long numSteps = 0;

		for (int x = 0; x < consoleSize.x; x++) {
			for (int y = 0; y < consoleSize.y; y++) {
				glm::vec3 ray;

				ray.x = (static_cast<float>(x) / static_cast<float>(consoleSize.x) - 0.5f) * 2; //aspect ratio of characters is 2:1
				ray.y = static_cast<float>(y) / static_cast<float>(consoleSize.y) - 0.5f;

				ray.z = -0.5f;
				
				glm::vec3 stepRay = glm::vec4(normalize(ray), 1.0f) * cam.GetViewMatrix();
				stepRay = normalize(stepRay);
				ray = cam.Position;
				float threshold = 1.f;

				//Cast ray
				float minDistance = INFINITY;
				for (float f = 0; f < 50.f; f += max(minDistance, threshold)) {
					//Calculate global SDF
					for (auto& obj : objects) {
						float dist = obj->signedDistance(ray);
						if (dist < minDistance)
							minDistance = dist;
					}

					ray += stepRay * minDistance;

					numSteps++;

					if (minDistance < threshold) {
						setChar({x,y}, '.');
						break;
					}
				}
			}
		}

		writeString({0,0}, std::to_string(cam.Position.x));
		writeString({0,1}, std::to_string(cam.Position.y));
		writeString({0,2}, std::to_string(cam.Position.z));
		writeString({0,3}, std::to_string(numSteps) + " steps");

		
		//Display screen
		screen[consoleSize.x * consoleSize.y - 1] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen.data(), consoleSize.x * consoleSize.y, { 0,0 }, &dwBytesWritten);

		std::chrono::duration<double> duration = std::chrono::system_clock::now() - start;
		delta = duration.count();
	}
}
