#include <iostream>
#include <Windows.h>
#include <stdio.h>
#include <chrono>
#include <vector>
#include <utility>
#include <algorithm>
using namespace std;


int screenWidth = 120;
int screenHeight = 40;
int mapHeight = 16;
int mapWidth = 16;

float playerX = 14.7f;
float playerY = 5.09f;
float playerA = 0.0f; // angle

float fov = 3.14159f / 4.0;  // field of view
float depth = 16.0f;  // max render distance
float speed = 3.0f;  // player walk speed

int main() {
	// screen buffer
	wchar_t* screen = new wchar_t[screenWidth * screenHeight];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	// map
	wstring map;

	// # = wall, . == space
	map += L"################";
	map += L"#.......#......#";
	map += L"#..............#";
	map += L"#....##....#...#";
	map += L"#....#.....#...#";
	map += L"#..###.........#";
	map += L"#..............#";
	map += L"#..###....######";
	map += L"#....#.........#";
	map += L"#....#....#....#";
	map += L"#....#....#....#";
	map += L"#.........#....#";
	map += L"###............#";
	map += L"#......#########";
	map += L"#..............#";
	map += L"################";

	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();

	// game loop
	while (1) {
		tp2 = chrono::system_clock::now();
		chrono::duration<float> time = tp2 - tp1;
		tp1 = tp2;
		float elapsedTime = time.count();

		// controls
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000) {  // left
			playerA -= (speed * 0.75f) * elapsedTime;
		}

		if (GetAsyncKeyState((unsigned short)'D') & 0x8000) {  // right
			playerA += (speed * 0.75f) * elapsedTime;
		}

		if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {  // forward
			playerX += sinf(playerA) * speed * elapsedTime;;
			playerY += cosf(playerA) * speed * elapsedTime;;

			// collision with wall
			if (map.c_str()[(int)playerX * mapWidth + (int)playerY] == '#') {
				playerX -= sinf(playerA) * speed * elapsedTime;;
				playerX -= cosf(playerA) * speed * elapsedTime;;
			}
		}

		if (GetAsyncKeyState((unsigned short)'S') & 0x8000) {  // backward
			playerX -= sinf(playerA) * speed * elapsedTime;;
			playerY -= cosf(playerA) * speed * elapsedTime;;

			// collision with wall
			if (map.c_str()[(int)playerX * mapWidth + (int)playerY] == '#') {
				playerX += sinf(playerA) * speed * elapsedTime;;
				playerX += cosf(playerA) * speed * elapsedTime;;
			}
		}

		for (int x = 0; x < screenWidth; x++) {
#			// calculate ray angle between player and col
			float rayAngle = (playerA - fov / 2.0f) + ((float)x / (float)screenWidth) * fov;

			// distance to wall
			float stepSize = 0.1f;  // inc size for ray, decr to incr res
			float distanceToWall = 0.0f;

			bool hitWall = false;
			bool boundary = false;

			// unit vector for ray in space
			float eyeX = sinf(rayAngle);
			float eyeY = cosf(rayAngle);

			while (!hitWall && distanceToWall < depth) {
				distanceToWall += stepSize;

				// calculates distance to wall
				int testX = (int)(playerX + eyeX * distanceToWall);
				int testY = (int)(playerY + eyeY * distanceToWall);

				// tests if ray is within bounds
				if (testX < 0 || testX >= mapWidth || testY < 0 || testY >= mapHeight) {  // outside bounds
					hitWall = true;
					distanceToWall = depth;
				} else {  // within bounds and tests if ray hits wall
					if (map.c_str()[testX * mapWidth + testY] == '#') {
						hitWall = true;

						vector<pair<float, float>> p; // distance, dot product of angles

						for (int tx = 0; tx < 2; tx++) {
							for (int ty = 0; ty < 2; ty++) {
								// perfect corner of cell
								float vx = (float)testX + tx - playerX;
								float vy = (float)testY + ty - playerY;

								float d = sqrt(vx * vx + vy * vy);
								float dot = (eyeX * vx / d) + (eyeY * vy / d);
								p.push_back(make_pair(d, dot));
							}
						}

						// Sort Pairs from closest to farthest
						sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) {return left.first < right.first; });

						// check if ray hits cell boundary
						float bound = 0.01;
						if (acos(p.at(0).second) < bound) { boundary = true; }
						if (acos(p.at(1).second) < bound) { boundary = true; }
					}
				}
			}

			// calculate distance to ceiling and floor (size increases with distance)
			int ceiling = (float)(screenHeight / 2.0) - screenHeight / ((float)distanceToWall);
			int floor = screenHeight - ceiling;

			short shade = ' ';

			if (distanceToWall <= depth / 4.0f) { shade = 0x2588; }  // very close
			else if (distanceToWall < depth / 3.0f) { shade = 0x2593; }
			else if (distanceToWall < depth / 2.0f) { shade = 0x2592; }
			else if (distanceToWall < depth) { shade = 0x2591; }
			else { shade = ' '; }  // very far

			if (boundary) { shade = ' '; }

			for (int y = 0; y < screenHeight; y++) {
				if (y <= ceiling) {  // ceiling
					screen[y * screenWidth + x] = ' ';
				} else if (y > ceiling && y <= floor) {  // wall
					screen[y * screenWidth + x] = shade;
				} else {  // floor
					// Shade floor based on distance
					float b = 1.0f - (((float)y - screenHeight / 2.0f) / ((float)screenHeight / 2.0f));
					if (b < 0.25) { shade = '#'; }
					else if (b < 0.5) { shade = 'x'; }
					else if (b < 0.75) { shade = '.'; }
					else if (b < 0.9) { shade = '-'; }
					else { shade = ' '; }
					screen[y * screenWidth + x] = shade;
				}
			}

		}

		// display stats
		swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", playerX, playerY, playerA, 1.0f / elapsedTime);

		// display map
		for (int mx = 0; mx < mapWidth; mx++) {
			for (int my = 0; my < mapWidth; my++) {
				screen[(my + 1) * screenWidth + mx] = map[my * mapWidth + mx];
			}
		}

		screen[((int)playerY + 1) * screenWidth + (int)playerX] = 'P';


		// write to screen
		// text written to top left corner
		screen[screenWidth * screenHeight - 1] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen, screenWidth * screenHeight, { 0, 0 }, &dwBytesWritten);
	}

	return 0;
}