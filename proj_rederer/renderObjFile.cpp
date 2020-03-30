#include "olcConsoleGameEngineSDL.h"
#include <fstream>
#include <strstream>
#include <algorithm>
using namespace std;

struct vec3d {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;
};

struct triangle {
    vec3d p[3];

    // Color of the triangle
    unsigned short  sym;
    short col;
};

struct mesh {
    vector<triangle> tris;

    bool LoadFromObjectFile(string sFileName) {
        
        ifstream file(sFileName);
        if(!file.is_open())
            return false;

        // Local cache of vertices
        vector<vec3d> verts;

        while(!file.eof()) {
            char line[128];
            file.getline(line, 128);

            strstream s;
            s << line;

            char junk;

            if(line[0] == 'v') {
                vec3d v;
                s >> junk >> v.x >> v.y >> v.z;
                verts.push_back(v);
            } else if (line[0] == 'f') {
                int f[3];
                s >> junk >> f[0] >> f[1] >> f[2];

                tris.push_back({ verts[f[0] -1], verts[f[1] - 1], verts[f[2] - 1] });
            }
        }


        return true;
    }
};

struct mat4x4 {
    float m[4][4] = { 0 };
};

class Engine3D : public olcConsoleGameEngine {
    private:
        string objFile;

        // Field of view
        float fTheta;
        
        // Observer Position
        vec3d vCamera;

        mesh meshCube;
        mat4x4 matProj;

        vec3d MatrixMultiplyVector(mat4x4 &m, vec3d &i) {
            vec3d v;

            v.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + i.w * m.m[3][0];
            v.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + i.w * m.m[3][1];
            v.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + i.w * m.m[3][2];
            v.w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + i.w * m.m[3][3];

            return v;
        }

        mat4x4 IdentityMatrix() {
            mat4x4 matrix;

            matrix.m[0][0] = 1.0f;
            matrix.m[1][1] = 1.0f;
            matrix.m[2][2] = 1.0f;
            matrix.m[3][3] = 1.0f;
        
            return matrix;
        }

        mat4x4 MatrixRotationX(float fAngleRad) {
            mat4x4 matrix;
            
            matrix.m[0][0] = 1.0f;
            matrix.m[1][1] = cosf(fAngleRad);
            matrix.m[1][2] = sinf(fAngleRad);
            matrix.m[2][1] = -sinf(fAngleRad);
            matrix.m[2][2] = cosf(fAngleRad);
            matrix.m[3][3] = 1.0f;

            return matrix;
        }

        mat4x4 MatrixRotationY(float fAngleRad) {
            mat4x4 matrix;
            
            matrix.m[0][0] = cosf(fAngleRad);
            matrix.m[0][2] = sinf(fAngleRad);
            matrix.m[2][0] = -sinf(fAngleRad);
            matrix.m[1][1] = 1.0f;
            matrix.m[2][2] = cosf(fAngleRad);
            matrix.m[3][3] = 1.0f;

            return matrix;
        }

        mat4x4 MatrixRotationZ(float fAngleRad) {
            mat4x4 matrix;
            
            matrix.m[0][0] = cosf(fAngleRad);
            matrix.m[0][1] = sinf(fAngleRad);
            matrix.m[1][0] = -sinf(fAngleRad);
            matrix.m[1][1] = cosf(fAngleRad);
            matrix.m[2][2] = 1.0f;
            matrix.m[3][3] = 1.0f;

            return matrix;
        }

        mat4x4 MatrixTranslation(float x, float y, float z) {
            mat4x4 matrix;
            
            matrix.m[0][0] = 1.0f;
            matrix.m[1][1] = 1.0f;
            matrix.m[2][2] = 1.0f;
            matrix.m[3][3] = 1.0f;
            matrix.m[3][0] = x;
            matrix.m[3][1] = y;
            matrix.m[3][2] = z;

            return matrix;
        }

        mat4x4 MatrixProjection(float fFovDegrees, float fAspectRatio, float fNear, float fFar) {
            float fFovRad = 1.0f  / tanf(fFovDegrees * 0.5f / 180.0f * 3.14159f);
            mat4x4 matrix;

            matrix.m[0][0] = fAspectRatio * fFovRad;
            matrix.m[1][1] = fFovRad;
            matrix.m[2][2] = fFar / (fFar - fNear);
            matrix.m[3][2] = (-fNear * fFar) / (fFar - fNear);
            matrix.m[2][3] = 1.0f;
            matrix.m[3][3] = 0.0f;
        
            return matrix;
        }

        mat4x4 MatrixMultiplyMatrix(mat4x4 &m1, mat4x4 &m2) {
            mat4x4 matrix;

            for(int c = 0; c < 4; c++) {
                for(int r = 0; r < 4; r++) {
                    matrix.m[r][c] = 
                        m1.m[r][0] * m2.m[0][c] + 
                        m1.m[r][1] * m2.m[1][c] + 
                        m1.m[r][2] * m2.m[2][c] + 
                        m1.m[r][3] * m2.m[3][c]
                    ;
                }
            }

            return matrix;
        }

        vec3d VectorAdd(vec3d &v1, vec3d &v2) {
            return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
        }

        vec3d VectorSub(vec3d &v1, vec3d &v2) {
            return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
        }

        vec3d VectorMul(vec3d &v1, float k) {
            return { v1.x * k, v1.y * k, v1.z * k };
        }

        vec3d VectorDiv(vec3d &v1, float k) {
            return { v1.x / k, v1.y / k, v1.z / k };
        }

        float DotProduct(vec3d &v1, vec3d &v2) {
            return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
        }

        float VectorLength(vec3d &v) {
            return sqrtf(DotProduct(v, v));
        }

        vec3d Normalise(vec3d &v) {
            float l = VectorLength(v);
            return { v.x / l, v.y / l, v.z / l };
        }

        vec3d CrossProduct(vec3d &v1, vec3d &v2) {
            vec3d v;

            v.x = v1.y * v2.z - v1.z * v2.y;
            v.y = v1.z * v2.x - v1.x * v2.z;
            v.z = v1.x * v2.y - v1.y * v2.x;

            return v;
        }

        // Taken From Command Line Webcam Video
        CHAR_INFO GetColour(float lum) {
            short bg_col, fg_col;
            wchar_t sym;
            int pixel_bw = (int)(13.0f*lum);
            switch (pixel_bw)
            {
            case 0: bg_col = BG_BLACK; fg_col = FG_BLACK; sym = PIXEL_SOLID; break;

            case 1: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_QUARTER; break;
            case 2: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_HALF; break;
            case 3: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_THREEQUARTERS; break;
            case 4: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_SOLID; break;

            case 5: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_QUARTER; break;
            case 6: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_HALF; break;
            case 7: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_THREEQUARTERS; break;
            case 8: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_SOLID; break;

            case 9:  bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_QUARTER; break;
            case 10: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_HALF; break;
            case 11: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_THREEQUARTERS; break;
            case 12: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_SOLID; break;
            default:
                bg_col = BG_BLACK; fg_col = FG_BLACK; sym = PIXEL_SOLID;
            }

            CHAR_INFO c;
            //c.Attributes = bg_col | fg_col;
            //c.Char.UnicodeChar = sym;
            c.colour = bg_col | fg_col;
            c.glyph = sym;
            
            //cout << bg_col << endl << fg_col << endl << sym << "\n\n";
            
            return c;
        }

    public:
        Engine3D(string _objFile) {
            objFile = _objFile;
            m_sAppName = L"3D Window";
        }

        bool OnUserCreate() override {
           
            meshCube.LoadFromObjectFile(objFile);

            // Projection Matrix
            matProj = MatrixProjection(90.0f, (float)ScreenHeight() / (float)ScreenWidth(), 0.1f, 1000.0f);

            return true;
        }

        bool OnUserUpdate(float fElapsedTime) override {
            // Clear the screen
            Fill(0 ,0, ScreenWidth(), ScreenHeight(), PIXEL_SOLID, FG_BLACK);

            // Roatation Matrices
            mat4x4 matRotZ, matRotX;
            fTheta += 1.0f * fElapsedTime;

            // Rotation Matrices
            matRotZ = MatrixRotationZ(fTheta * 0.5f);
            matRotX = MatrixRotationX(fTheta);

            // Translation Matrix
            mat4x4 matTrans;
		    matTrans = MatrixTranslation(0.0f, 0.0f, 10.0f);

            // World Matrix
		    mat4x4 matWorld;
            matWorld = IdentityMatrix();
            matWorld = MatrixMultiplyMatrix(matRotZ, matRotX); // Transform by rotation
            matWorld = MatrixMultiplyMatrix(matWorld, matTrans); // Transform by translation

            vector<triangle> vecTrianglesToRaster;

            // Draw triangles
            for(auto tri : meshCube.tris) {

                triangle triProjected, triTransformed;

                // World Matrix Transform
                triTransformed.p[0] = MatrixMultiplyVector(matWorld, tri.p[0]);
                triTransformed.p[1] = MatrixMultiplyVector(matWorld, tri.p[1]);
                triTransformed.p[2] = MatrixMultiplyVector(matWorld, tri.p[2]);

                // Calculate triangle normal
                vec3d normal, line1, line2;

                // Get lines either side of triangle
                line1 = VectorSub(triTransformed.p[1], triTransformed.p[0]);
                line2 = VectorSub(triTransformed.p[2], triTransformed.p[0]);

                // Unit normal to the trinagle surface
                normal = CrossProduct(line1, line2);
                normal = Normalise(normal);

                // Ray from tringle to camera
                vec3d vCameraRay = VectorSub(triTransformed.p[0], vCamera);

                if(DotProduct(normal, vCameraRay) < 0.0f){
                    // Illumination
                    vec3d lightDirection = { 0.0f, 0.0f, -1.0f };
                    lightDirection = Normalise(lightDirection);
                    
                    // How "aligned" are light direction and triangle surface normal?
                    float dp = max(0.1f, DotProduct(lightDirection, normal));; 

                    CHAR_INFO c = GetColour(dp);
                    triTransformed.col = c.colour;
                    triTransformed.sym = c.glyph;

                    // Project triangles from 3D --> 2D
                    triProjected.p[0] = MatrixMultiplyVector(matProj, triTransformed.p[0]);
                    triProjected.p[1] = MatrixMultiplyVector(matProj, triTransformed.p[1]);
                    triProjected.p[2] = MatrixMultiplyVector(matProj, triTransformed.p[2]);
                    triProjected.col = triTransformed.col;
                    triProjected.sym = triTransformed.sym;

                    // Scale into view
                    triProjected.p[0] = VectorDiv(triProjected.p[0], triProjected.p[0].w);
                    triProjected.p[1] = VectorDiv(triProjected.p[1], triProjected.p[1].w);
                    triProjected.p[2] = VectorDiv(triProjected.p[2], triProjected.p[2].w);

                    // Offset the screen space to the center of the screen 
                    vec3d vOffsetView = { 1, 1, 0 };
                    triProjected.p[0] = VectorAdd(triProjected.p[0], vOffsetView);
                    triProjected.p[1] = VectorAdd(triProjected.p[1], vOffsetView);
                    triProjected.p[2] = VectorAdd(triProjected.p[2], vOffsetView);

                    triProjected.p[0].x *= 0.5f * (float)ScreenWidth();
                    triProjected.p[0].y *= 0.5f * (float)ScreenHeight();
                    triProjected.p[1].x *= 0.5f * (float)ScreenWidth();
                    triProjected.p[1].y *= 0.5f * (float)ScreenHeight();
                    triProjected.p[2].x *= 0.5f * (float)ScreenWidth();
                    triProjected.p[2].y *= 0.5f * (float)ScreenHeight();

                    // Rasterize triangle
                    vecTrianglesToRaster.push_back(triProjected);
                }
            }

            // Sort tringles from back to front
            sort(
                vecTrianglesToRaster.begin(),
                vecTrianglesToRaster.end(),
                [](triangle &t1, triangle &t2) {
                    float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
                    float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;

                    return z1 > z2;
                }
            );

            for(auto &triProjected : vecTrianglesToRaster) {
                FillTriangle(
                    triProjected.p[0].x, triProjected.p[0].y,
                    triProjected.p[1].x, triProjected.p[1].y,
                    triProjected.p[2].x, triProjected.p[2].y,
                    triProjected.sym, triProjected.col
                );
                // Draw triangular structure
                //DrawTriangle(
                //    triProjected.p[0].x, triProjected.p[0].y,
                //    triProjected.p[1].x, triProjected.p[1].y,
                //    triProjected.p[2].x, triProjected.p[2].y,
                //    PIXEL_SOLID, FG_DARK_CYAN
                //);
            }

            return true;
        }

};

int main(int argc, char **argv) {

    if(argc < 2) {
        cout << "Missing Arguments!\nUsage: " << argv[0] << " objFile.obj\n";
        exit(-1);
    }

    Engine3D engine(argv[1]);

    if(engine.ConstructConsole(250, 200, 3, 3)) 
        engine.Start();
    else {
        cout << "Not able to start window!\n";
        exit(-1);
    }

    return 0;
}