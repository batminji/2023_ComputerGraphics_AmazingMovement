#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <time.h>
#include <stdlib.h>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <fstream>
#include <Windows.h>

#define WINDOWX 1000
#define WINDOWY 1000

using namespace std;

random_device rd;
default_random_engine dre(rd());
uniform_real_distribution<float>uid(0.0f, 1.0f);
uniform_real_distribution<float>uid_vel(0.0001f, 0.005f);
uniform_int_distribution<int>uid_dir(0, 1);
uniform_real_distribution<float>uid_min_h(0.0001f, 0.1f);
uniform_real_distribution<float>uid_max_h(0.9f, 1.0f);

void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid InitBuffer();
void InitShader();
GLchar* filetobuf(const char* file);
void LoadOBJ(const char* filename, vector<glm::vec3>& out_vertex, vector<glm::vec2>& out_uvs, vector<glm::vec3>& out_normals);

GLuint shaderID;
GLint width, height;

GLuint vertexShader;
GLuint fragmentShader;

GLuint VAO, VBO[2];

class Plane {
public:
    vector<glm::vec3> vertex;
    vector<glm::vec3> color;
    vector<glm::vec3> normals;
    vector<glm::vec2> uvs;

    void Bind() {
        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferData(GL_ARRAY_BUFFER, vertex.size() * sizeof(glm::vec3), vertex.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);
    }
    void Draw() {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertex.size());
    }
};
Plane Cube;

class MoveCube {
public:
    float tx, ty, tz;
    float velocity;
    float min_height, max_height;
    int move_direction;
};
MoveCube moveCube[25][25];

float BackGround[] = { 0.0, 0.0, 0.0 };

glm::vec4* vertex;
glm::vec4* face;
glm::vec3* outColor;

glm::mat4 TR = glm::mat4(1.0f);

FILE* FL;
int faceNum = 0;
bool start = true;

void keyboard(unsigned char, int, int);
void Mouse(int button, int state, int x, int y);
void Motion(int x, int y);
void TimerFunction(int value);
GLvoid drawScene(GLvoid);
GLvoid Reshape(int w, int h);

void make_vertexShaders()
{

    GLchar* vertexShaderSource;

    vertexShaderSource = filetobuf("vertex.glsl");

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint result;
    GLchar errorLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
        cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << endl;
        exit(-1);
    }
}

void make_fragmentShaders()
{
    const GLchar* fragmentShaderSource = filetobuf("fragment.glsl");

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLint result;
    GLchar errorLog[512];
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
        cerr << "ERROR: fragment shader 컴파일 실패\n" << errorLog << endl;
        exit(-1);
    }

}

GLuint make_shaderProgram()
{
    GLint result;
    GLchar errorLog[512];
    GLuint ShaderProgramID;
    ShaderProgramID = glCreateProgram(); //--- 세이더 프로그램 만들기
    glAttachShader(ShaderProgramID, vertexShader); //--- 세이더 프로그램에 버텍스 세이더 붙이기
    glAttachShader(ShaderProgramID, fragmentShader); //--- 세이더 프로그램에 프래그먼트 세이더 붙이기
    glLinkProgram(ShaderProgramID); //--- 세이더 프로그램 링크하기

    glDeleteShader(vertexShader); //--- 세이더 객체를 세이더 프로그램에 링크했음으로, 세이더 객체 자체는 삭제 가능
    glDeleteShader(fragmentShader);

    glGetProgramiv(ShaderProgramID, GL_LINK_STATUS, &result); // ---세이더가 잘 연결되었는지 체크하기
    if (!result) {
        glGetProgramInfoLog(ShaderProgramID, 512, NULL, errorLog);
        cerr << "ERROR: shader program 연결 실패\n" << errorLog << endl;
        exit(-1);
    }
    glUseProgram(ShaderProgramID); //--- 만들어진 세이더 프로그램 사용하기
    //--- 여러 개의 세이더프로그램 만들 수 있고, 그 중 한개의 프로그램을 사용하려면
    //--- glUseProgram 함수를 호출하여 사용 할 특정 프로그램을 지정한다.
    //--- 사용하기 직전에 호출할 수 있다.
    return ShaderProgramID;
}

void InitShader()
{
    make_vertexShaders(); //--- 버텍스 세이더 만들기
    make_fragmentShaders(); //--- 프래그먼트 세이더 만들기
    shaderID = make_shaderProgram(); //--- 세이더 프로그램 만들기
}

GLvoid InitBuffer() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(2, VBO);
    glBindVertexArray(VAO);
}

float camera_z = 5.0f, camera_x = 0.0f;
int camera_rotate = 0;
float camera_rotate_y = 0.0f;
float rotate_y = 0.0f;
bool camera_cw_rotate = false, camera_ccw_rotate = false;

float light_x = 0.0f, light_z = 2.0f;
int light_color = 0;
bool light_switch = true;

int width_num = 0;
int height_num = 0;
int animation_mode = 0;

int timer_speed = 10;
int wave_cnt = 0;
int wave_start = 0;

int max_width = 0, max_height = 0;
int min_width = 0, min_height = 0;
int animation3_i = 0, animation3_j = 0;
int animation3_dir = 0;

GLvoid drawScene() //--- 콜백 함수: 그리기 콜백 함수 
{
    if (start) {
        start = FALSE;
        LoadOBJ("cube.obj", Cube.vertex, Cube.uvs, Cube.normals);

        for (int i = 0; i < 25; ++i) {
            for (int j = 0; j < 25; ++j) {
                moveCube[i][j].tx = -0.92f + j * 0.08;
                moveCube[i][j].ty = 0.0f;
                moveCube[i][j].tz = -0.92f + i * 0.08;
                moveCube[i][j].velocity = uid_vel(dre);
                moveCube[i][j].min_height = uid_min_h(dre);
                moveCube[i][j].max_height = uid_max_h(dre);
                moveCube[i][j].move_direction = 1;
            }
        }

        glEnable(GL_DEPTH_TEST);

    } // 초기화할 데이터

    glViewport(0, 0, 1000, 1000);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);    //배경

    glUseProgram(shaderID);
    glBindVertexArray(VAO);// 쉐이더 , 버퍼 배열 사용

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    unsigned int modelLocation = glGetUniformLocation(shaderID, "model");
    unsigned int viewLocation = glGetUniformLocation(shaderID, "view");
    unsigned int projLocation = glGetUniformLocation(shaderID, "projection");

    unsigned int lightPosLocation = glGetUniformLocation(shaderID, "lightPos");
    unsigned int lightColorLocation = glGetUniformLocation(shaderID, "lightColor");
    unsigned int objColorLocation = glGetUniformLocation(shaderID, "objectColor");
    glm::mat4 Vw = glm::mat4(1.0f);
    glm::mat4 Pj = glm::mat4(1.0f);
    glm::mat4 Cp = glm::mat4(1.0f);
    glm::vec3 cameraPos;
    glm::vec3 cameraDirection;
    glm::vec3 cameraUp;

    Cp = glm::rotate(Cp, (float)glm::radians(rotate_y), glm::vec3(0.0f, 1.0f, 0.0f));
    cameraPos = glm::vec4(2.0f, 3.5f, 2.0f, 1.0f) * Cp; //--- 카메라 위치
    cameraDirection = glm::vec3(0.0f, 0.5f, 0.0f); //--- 카메라 바라보는 방향
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f); //--- 카메라 위쪽 방향
    Vw = glm::mat4(1.0f);
    Vw = glm::lookAt(cameraPos, cameraDirection, cameraUp);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &Vw[0][0]);

    Pj = glm::mat4(1.0f);
    Pj = glm::perspective(glm::radians(60.0f), (float)WINDOWX / (float)WINDOWY, 0.1f, 200.0f);
    glUniformMatrix4fv(projLocation, 1, GL_FALSE, &Pj[0][0]);

    glUniform3f(lightPosLocation, 0.0f, 2.0f, 0.0f); //--- lightPos 값 전달: (0.0, 0.0, 5.0);
    if (light_switch) {
        switch (light_color) {
        case 0:
            glUniform3f(lightColorLocation, 1.0, 1.0, 1.0);
            break;
        case 1:
            glUniform3f(lightColorLocation, 0.968f, 0.039f, 0.039f);
            break;
        case 2:
            glUniform3f(lightColorLocation, 0.988f, 0.917, 0.109f);
            break;
        case 3:
            glUniform3f(lightColorLocation, 0.109f, 0.988f, 0.239f);
            break;
        }
    }
    else {
        glUniform3f(lightColorLocation, 0.1f, 0.1f, 0.1f);
    }
    // 그리기 코드 
    // 부모행렬 -> 공전 -> 이동 -> 자전 -> 스케일
    
    // 바닥
    TR = glm::mat4(1.0f);
    TR = glm::translate(TR, glm::vec3(0.0f, 0.0f, 0.0f));
    TR = glm::scale(TR, glm::vec3(1.0f, 0.01f, 1.0f));
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));

    glUniform3f(objColorLocation, 0.1f, 0.1f, 0.1f); //--- object Color값 전달: (1.0, 0.5, 0.3)의 색

    Cube.Bind();
    Cube.Draw();

    // 
    for (int i = 0; i < height_num; ++i) {
        for (int j = 0; j < width_num; ++j) {
            TR = glm::mat4(1.0f);
            TR = glm::translate(TR, glm::vec3(moveCube[i][j].tx, moveCube[i][j].ty, moveCube[i][j].tz));
            TR = glm::scale(TR, glm::vec3(0.04, moveCube[i][j].ty, 0.04));
            glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));

            glUniform3f(objColorLocation, 0.529f, 0.784f, 0.980f);

            Cube.Bind();
            Cube.Draw();
        }
    }

    ////////////////////////////////미니맵///////////////////////////////////////
    glViewport(800, 800, 200, 200);

    
    modelLocation = glGetUniformLocation(shaderID, "model");
    viewLocation = glGetUniformLocation(shaderID, "view");
    projLocation = glGetUniformLocation(shaderID, "projection");

    lightPosLocation = glGetUniformLocation(shaderID, "lightPos");
    lightColorLocation = glGetUniformLocation(shaderID, "lightColor");
    objColorLocation = glGetUniformLocation(shaderID, "objectColor");
    Vw = glm::mat4(1.0f);
    Pj = glm::mat4(1.0f);

    cameraPos = glm::vec4(0.0f, 5.0f, 0.0f, 1.0f); //--- 카메라 위치
    cameraDirection = glm::vec3(0.0f, 0.0f, 0.0f); //--- 카메라 바라보는 방향
    cameraUp = glm::vec3(-1.0f, 0.0f, 0.0f); //--- 카메라 위쪽 방향
    Vw = glm::mat4(1.0f);
    Vw = glm::lookAt(cameraPos, cameraDirection, cameraUp);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &Vw[0][0]);

    Pj = glm::mat4(1.0f);
    Pj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -100.0f, 100.0f);
    glUniformMatrix4fv(projLocation, 1, GL_FALSE, &Pj[0][0]);

    glUniform3f(lightPosLocation, 0.0f, 2.0f, 0.0f); //--- lightPos 값 전달: (0.0, 0.0, 5.0);
    if (light_switch) {
        switch (light_color) {
        case 0:
            glUniform3f(lightColorLocation, 1.0, 1.0, 1.0);
            break;
        case 1:
            glUniform3f(lightColorLocation, 0.968f, 0.039f, 0.039f);
            break;
        case 2:
            glUniform3f(lightColorLocation, 0.988f, 0.917, 0.109f);
            break;
        case 3:
            glUniform3f(lightColorLocation, 0.109f, 0.988f, 0.239f);
            break;
        }
    }
    else {
        glUniform3f(lightColorLocation, 0.1f, 0.1f, 0.1f);
    }
    // 그리기 코드 
    // 부모행렬 -> 공전 -> 이동 -> 자전 -> 스케일

    // 바닥
    TR = glm::mat4(1.0f);
    TR = glm::translate(TR, glm::vec3(0.0f, 0.0f, 0.0f));
    TR = glm::scale(TR, glm::vec3(1.0f, 0.01f, 1.0f));
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));

    glUniform3f(objColorLocation, 0.1f, 0.1f, 0.1f); //--- object Color값 전달: (1.0, 0.5, 0.3)의 색

    Cube.Bind();
    Cube.Draw();

    // 
    for (int i = 0; i < height_num; ++i) {
        for (int j = 0; j < width_num; ++j) {
            TR = glm::mat4(1.0f);
            TR = glm::translate(TR, glm::vec3(moveCube[i][j].tx, moveCube[i][j].ty, moveCube[i][j].tz));
            TR = glm::scale(TR, glm::vec3(0.04, moveCube[i][j].ty, 0.04));
            glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(TR));

            glUniform3f(objColorLocation, 0.529f, 0.784f, 0.980f);

            Cube.Bind();
            Cube.Draw();
        }
    }


    glutSwapBuffers();
    glutPostRedisplay();
}

GLvoid Reshape(int w, int h) //--- 콜백 함수: 다시 그리기 콜백 함수
{

}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case '1':
        animation_mode = 0;
        break;
    case '2':
        animation_mode = 1;
        for (int i = 0; i < height_num; ++i) {
            for (int j = 0; j < width_num; ++j) {
                moveCube[i][j].move_direction = -1;
                moveCube[i][j].ty = 0.2f;
            }
        }
        wave_start = 0; wave_cnt = 0;
        break;
    case '3':
        animation_mode = 2;
        for (int i = 0; i < height_num; ++i) {
            for (int j = 0; j < width_num; ++j) {
                moveCube[i][j].move_direction = -1;
                moveCube[i][j].ty = 0.2f;
            }
        }
        max_width = width_num - 1; max_height = height_num - 1;
        min_width = 0; min_height = 0;
        animation3_i = 0; animation3_j = 0; animation3_dir = 0; wave_cnt = 0;
        break;
    case 't':
        light_switch = !light_switch;
        break;
    case 'c':
        light_color = (light_color + 1) % 4;
        break;
    case 'y':
        camera_cw_rotate = !camera_cw_rotate;
        camera_ccw_rotate = false;
        break;
    case 'Y':
        camera_cw_rotate = false;
        camera_ccw_rotate = !camera_ccw_rotate;
        break;
    case '+':
        if(timer_speed > 1)timer_speed -= 1;
        break;
    case '-':
        timer_speed += 1;
        break;
    case 'r':
        system("cls");
        timer_speed = 0;
        width_num = 0; height_num = 0;
        while (width_num < 5 || width_num > 25) {
            cout << "가로 육면체 개수 : ";
            cin >> width_num;
        }
        while (height_num < 5 || height_num > 25) {
            cout << "세로 육면체 개수 : ";
            cin >> height_num;
        }
        system("cls");
        timer_speed = 10; camera_cw_rotate = false; camera_ccw_rotate = false; animation_mode = 0; rotate_y = 0.0f;
        cout << "1 : 애니메이션 1" << endl
            << "2 : 애니메이션 2" << endl
            << "3 : 애니메이션 3" << endl
            << "t : 조명을 켠다 / 끈다" << endl
            << "y : 카메라가 바닥의 y축을 기준으로 양 방향으로 회전한다" << endl
            << "Y : 카메라가 바닥의 y축을 기준으로 음 방향으로 회전한다" << endl
            << "+ : 육면체 이동하는 속도 증가" << endl
            << "- : 육면체 이동하는 속도 감소" << endl
            << "r : 모든 값 초기화" << endl
            << "q : 프로그램 종료" << endl;
        break;
    case 'q':
        glutDestroyWindow(1);
        break;
    }
    glutPostRedisplay();
}

void TimerFunction(int value)
{
    // 애니메이션
    switch (animation_mode) {
    case 0:
        for (int i = 0; i < height_num; ++i) {
            for (int j = 0; j < width_num; ++j) {
                if (moveCube[i][j].move_direction == 1) {
                    moveCube[i][j].ty += moveCube[i][j].velocity;
                    if (moveCube[i][j].ty >= moveCube[i][j].max_height)moveCube[i][j].move_direction = 0;
                }
                else {
                    moveCube[i][j].ty -= moveCube[i][j].velocity;
                    if (moveCube[i][j].ty <= moveCube[i][j].min_height)moveCube[i][j].move_direction = 1;
                }
            }
        }
        break;
    case 1:
        for (int i = 0; i < height_num; ++i) {
            for (int j = 0; j < width_num; ++j) {
                if (moveCube[i][j].move_direction == 1) {
                    moveCube[i][j].ty += 0.005f;
                    if (moveCube[i][j].ty >= 1.0f)moveCube[i][j].move_direction = 0;
                }
                else if (moveCube[i][j].move_direction == 0) {
                    moveCube[i][j].ty -= 0.005f;
                    if (moveCube[i][j].ty <= 0.2f)moveCube[i][j].move_direction = 1;
                }
                else;
            }
        }
        if (wave_start < height_num) {
            wave_cnt++;
            if (wave_cnt == 8) {
                for (int i = 0; i < width_num; ++i) {
                    moveCube[wave_start][i].move_direction = 1;
                }
                wave_start++; wave_cnt = 0;
            }
        }
        break;
    case 2:
        for (int i = 0; i < height_num; ++i) {
            for (int j = 0; j < width_num; ++j) {
                if (moveCube[i][j].move_direction == 1) {
                    moveCube[i][j].ty += 0.005f;
                    if (moveCube[i][j].ty >= 1.0f)moveCube[i][j].move_direction = 0;
                }
                else if (moveCube[i][j].move_direction == 0) {
                    moveCube[i][j].ty -= 0.005f;
                    if (moveCube[i][j].ty <= 0.0f)moveCube[i][j].move_direction = 1;
                }
                else;
            }
        }

        if (min_width <= max_width && min_height <= max_height) {
            wave_cnt++;
            if (wave_cnt == 8) {
                switch (animation3_dir) {
                case 0: // 오른쪽으로
                    moveCube[animation3_i][animation3_j].move_direction = 1;
                    animation3_j++;
                    if (animation3_j == max_width) {
                        animation3_dir = 1;
                        max_width -= 1;
                    }
                    break;
                case 1: // 아래로
                    moveCube[animation3_i][animation3_j].move_direction = 1;
                    animation3_i++;
                    if (animation3_i == max_height) {
                        animation3_dir = 2;
                        max_height -= 1;
                    }
                    break;
                case 2: // 왼쪽으로
                    moveCube[animation3_i][animation3_j].move_direction = 1;
                    animation3_j--;
                    if (animation3_j == min_width) {
                        animation3_dir = 3;
                        min_width += 1;
                    }
                    break;
                case 3: // 위로
                    moveCube[animation3_i][animation3_j].move_direction = 1;
                    animation3_i--;
                    if (animation3_i == min_height) {
                        animation3_dir = 0;
                        min_height += 1;
                    }
                    break;
                }
                wave_cnt = 0;
            }
        }
        break;
    }

    // 카메라 회전
    if (camera_cw_rotate) {
        rotate_y -= 0.1f;
    }
    else if (camera_ccw_rotate) {
        rotate_y += 0.1f;
    }

    glutPostRedisplay();
    glutTimerFunc(timer_speed, TimerFunction, 1);
}

double mx, my;
void Mouse(int button, int state, int x, int y)
{
    mx = ((double)x - WINDOWX / 2.0) / (WINDOWX / 2.0);
    my = -(((double)y - WINDOWY / 2.0) / (WINDOWY / 2.0));


}
void Motion(int x, int y)
{
    mx = ((double)x - WINDOWX / 2.0) / (WINDOWX / 2.0);
    my = -(((double)y - WINDOWY / 2.0) / (WINDOWY / 2.0));

}

GLchar* filetobuf(const char* file) {
    FILE* fptr;
    long length;
    char* buf;
    fptr = fopen(file, "rb");
    if (!fptr)
        return NULL;
    fseek(fptr, 0, SEEK_END);
    length = ftell(fptr);
    buf = (char*)malloc(length + 1);
    fseek(fptr, 0, SEEK_SET);
    fread(buf, length, 1, fptr);
    fclose(fptr);
    buf[length] = 0;
    return buf;
}

void LoadOBJ(const char* filename, vector<glm::vec3>& out_vertex, vector<glm::vec2>& out_uvs, vector<glm::vec3>& out_normals)
{
    vector<int> vertexindices, uvindices, normalindices;
    vector<GLfloat> temp_vertex;
    vector<GLfloat> temp_uvs;
    vector<GLfloat> temp_normals;
    ifstream in(filename, ios::in);
    if (in.fail()) {
        cout << "Impossible to open file" << endl;
        return;
    }
    while (!in.eof()) {
        string lineHeader;
        in >> lineHeader;
        if (lineHeader == "v") {
            glm::vec3 vertex;
            in >> vertex.x >> vertex.y >> vertex.z;
            temp_vertex.push_back(vertex.x);
            temp_vertex.push_back(vertex.y);
            temp_vertex.push_back(vertex.z);
        }
        else if (lineHeader == "vt") {
            glm::vec2 uv;
            in >> uv.x >> uv.y;
            temp_uvs.push_back(uv.x);
            temp_uvs.push_back(uv.y);
        }
        else if (lineHeader == "vn") {
            glm::vec3 normal;
            in >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal.x);
            temp_normals.push_back(normal.y);
            temp_normals.push_back(normal.z);
        }
        else if (lineHeader == "f") {
            string vertex1, vertex2, vertex3;
            unsigned int vertexindex[3], uvindex[3], normalindex[3];
            for (int k = 0; k < 3; ++k) {
                string temp, temp2;
                int cnt{ 0 }, cnt2{ 0 };
                in >> temp;
                while (1) {
                    while ((int)temp[cnt] != 47 && cnt < temp.size()) {
                        temp2 += (int)temp[cnt];
                        cnt++;
                    }
                    if ((int)temp[cnt] == 47 && cnt2 == 0) {
                        vertexindex[k] = atoi(temp2.c_str());
                        vertexindices.push_back(vertexindex[k]);
                        cnt++; cnt2++;
                        temp2.clear();
                    }
                    else if ((int)temp[cnt] == 47 && cnt2 == 1) {
                        uvindex[k] = atoi(temp2.c_str());
                        uvindices.push_back(uvindex[k]);
                        cnt++; cnt2++;
                        temp2.clear();
                    }
                    else if (temp[cnt] = '\n' && cnt2 == 2) {
                        normalindex[k] = atoi(temp2.c_str());
                        normalindices.push_back(normalindex[k]);
                        break;
                    }
                }
            }
        }
        else {
            continue;
        }
    }
    for (int i = 0; i < vertexindices.size(); ++i) {
        unsigned int vertexIndex = vertexindices[i];
        vertexIndex = (vertexIndex - 1) * 3;
        glm::vec3 vertex = { temp_vertex[vertexIndex], temp_vertex[vertexIndex + 1], temp_vertex[vertexIndex + 2] };
        out_vertex.push_back(vertex);
    }
    for (unsigned int i = 0; i < uvindices.size(); ++i) {
        unsigned int uvIndex = uvindices[i];
        uvIndex = (uvIndex - 1) * 2;
        glm::vec2 uv = { temp_uvs[uvIndex], temp_uvs[uvIndex + 1] };
        out_uvs.push_back(uv);
    }
    for (unsigned int i = 0; i < normalindices.size(); ++i) {
        unsigned int normalIndex = normalindices[i];
        normalIndex = (normalIndex - 1) * 3;
        glm::vec3 normal = { temp_normals[normalIndex], temp_normals[normalIndex + 1], temp_normals[normalIndex + 2] };
        out_normals.push_back(normal);
    }
}

void main(int argc, char** argv) //--- 윈도우 출력하고 콜백함수 설정 { //--- 윈도우 생성하기
{
    while (width_num < 5 || width_num > 25) {
        cout << "가로 육면체 개수 : ";
        cin >> width_num;
    }
    while (height_num < 5 || height_num > 25) {
        cout << "세로 육면체 개수 : ";
        cin >> height_num;
    }
    system("cls");
    cout << "1 : 애니메이션 1" << endl
        << "2 : 애니메이션 2" << endl
        << "3 : 애니메이션 3" << endl
        << "t : 조명을 켠다 / 끈다" << endl
        << "y : 카메라가 바닥의 y축을 기준으로 양 방향으로 회전한다" << endl
        << "Y : 카메라가 바닥의 y축을 기준으로 음 방향으로 회전한다" << endl
        << "+ : 육면체 이동하는 속도 증가" << endl
        << "- : 육면체 이동하는 속도 감소" << endl
        << "r : 모든 값 초기화" << endl
        << "q : 프로그램 종료" << endl;


    srand((unsigned int)time(NULL));
    glutInit(&argc, argv); // glut 초기화
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // 디스플레이 모드 설정
    glutInitWindowPosition(0, 0); // 윈도우의 위치 지정
    glutInitWindowSize(WINDOWX, WINDOWY); // 윈도우의 크기 지정
    glutCreateWindow("Amazing Movement");// 윈도우 생성   (윈도우 이름)
    //--- GLEW 초기화하기


    glewExperimental = GL_TRUE;
    glewInit();

    InitShader();
    InitBuffer();

    glutKeyboardFunc(keyboard);
    glutTimerFunc(timer_speed, TimerFunction, 1);
    glutDisplayFunc(drawScene); //--- 출력 콜백 함수
    glutReshapeFunc(Reshape);
    glutMouseFunc(Mouse);
    glutMotionFunc(Motion);
    glutMainLoop();
}