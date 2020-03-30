#include "olcConsoleGameEngineSDL.h"
#include <stack>
using namespace std;

class Maze : public olcConsoleGameEngine {
    private:
        int mWidth, mHeight, visitedCells, pathWidth;
        int *maze;

        enum {
            CELL_PATH_N = 0x01,
            CELL_PATH_E = 0x02,
            CELL_PATH_S = 0x04,
            CELL_PATH_W = 0x08,
            CELL_VISITED = 0x10,
        };

        stack<pair<int, int>> mStack;

    public:
        Maze() {
            m_sAppName = L"Maze Generator";
        }

        bool OnUserCreate() override {
            // Maze parameters
            mWidth = 40;
            mHeight = 25;

            maze = new int[mWidth * mHeight];
            memset(maze, 0x00, mWidth * mHeight * sizeof(int));

            mStack.push(make_pair(0, 0));
            maze[0] = CELL_VISITED;
            visitedCells = 1;

            pathWidth = 3;

            return true;
        }

        bool OnUserUpdate(float fElapsedTime) override {
            // Delay animation
            this_thread::sleep_for(10ms);

            // Lambda func to offset coords
            auto offset = [&](int x, int y) {
                return (mStack.top().second + y) * mWidth + (mStack.top().first + x);
            };

            // Maze algorithm
            if(visitedCells < mWidth * mHeight) {
                // Create a set of the unvisited neighbours
                vector<int> neighbours;
                
                // North neighbour
                if (mStack.top().second > 0 && (maze[offset(0, -1)] & CELL_VISITED) == 0)
                    neighbours.push_back(0);
                // East neighbour
                if (mStack.top().first < mWidth - 1 && (maze[offset(1, 0)] & CELL_VISITED) == 0)
                    neighbours.push_back(1);
                // South neighbour
                if (mStack.top().second < mHeight - 1 && (maze[offset(0, 1)] & CELL_VISITED) == 0)
                    neighbours.push_back(2);
                // West neighbour
                if (mStack.top().first > 0 && (maze[offset(-1, 0)] & CELL_VISITED) == 0)
                    neighbours.push_back(3);

                if(!neighbours.empty()) {
                    // Choose one random neighbour
                    int nextCellDir = neighbours[rand() % neighbours.size()];

                    // Create a path between the neighbour and the current cell
                    switch (nextCellDir) {
                        case 0: // North
                            maze[offset(0, -1)] |= CELL_VISITED | CELL_PATH_S;
					        maze[offset(0,  0)] |= CELL_PATH_N;
                            mStack.push(make_pair((mStack.top().first + 0), (mStack.top().second - 1)));
                            break;

                        case 1: // East
                            maze[offset(+1, 0)] |= CELL_VISITED | CELL_PATH_W;
					        maze[offset( 0, 0)] |= CELL_PATH_E;
                            mStack.push(make_pair((mStack.top().first + 1), (mStack.top().second + 0)));
                            break;
                        
                        case 2: // South
                            maze[offset(0, +1)] |= CELL_VISITED | CELL_PATH_N;
					        maze[offset(0,  0)] |= CELL_PATH_S;
                            mStack.push(make_pair((mStack.top().first + 0), (mStack.top().second + 1)));
                            break;
                        
                        case 3: // West
                            maze[offset(-1, 0)] |= CELL_VISITED | CELL_PATH_E;
					        maze[offset( 0, 0)] |= CELL_PATH_W;
                            mStack.push(make_pair((mStack.top().first - 1), (mStack.top().second + 0)));
                            break;
                    }

                    visitedCells++;

                }else {
                    // Backtrack
                    mStack.pop();
                }

                
            }

            // Clears the Screen
            Fill(0, 0, ScreenWidth(), ScreenHeight(), L' ');

            // Draw maze
            for(int x = 0; x < mWidth; x++) {
                for(int y = 0; y < mHeight; y++) {

                    // Draw cells
                    for(int py = 0; py < pathWidth; py++) {
                        for(int px = 0; px < pathWidth; px++) {
                            if(maze[y * mWidth + x] & CELL_VISITED)
                                Draw(x * (pathWidth + 1) + px, y * (pathWidth + 1) + py, PIXEL_SOLID, FG_WHITE);
                            else
                                Draw(x * (pathWidth + 1) + px, y * (pathWidth + 1) + py, PIXEL_SOLID, FG_BLUE);
                        }
                    }

                    // Draw walls
                    for(int p = 0; p < pathWidth; p++) {
                        if(maze[y * mWidth + x] & CELL_PATH_S)
                            Draw(x * (pathWidth + 1) + p, y * (pathWidth + 1) + pathWidth);
                        if(maze[y * mWidth + x] & CELL_PATH_E)
                            Draw(x * (pathWidth + 1) + pathWidth, y * (pathWidth + 1) + p);
                    }

                }
            }

            // Draw Unit - the top of the stack
		    for (int py = 0; py < pathWidth; py++)
			    for (int px = 0; px < pathWidth; px++)
                    Draw(mStack.top().first * (pathWidth + 1) + px, mStack.top().second * (pathWidth + 1) + py, 0x2588, FG_GREEN); // Draw Cell

            return true;
        }

};

int main(int argc, char **argv) {
    
    Maze maze;

    if(maze.ConstructConsole(160, 100, 8, 8)) 
        maze.Start();
    else {
        cout << "Not able to start window!\n";
        exit(-1);
    }

    return 0;
}