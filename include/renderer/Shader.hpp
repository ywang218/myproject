// #ifndef SHADER_H
// #define SHADER_H

// #include <glad/glad.h>
// #include <string>
// #include <fstream>
// #include <sstream>
// #include <iostream>

// class Shader {
// public:
//     GLuint ID;

//     Shader(const char* vertexPath, const char* fragmentPath) {
//         // 1. 从文件路径读取源代码
//         std::string vertexCode;
//         std::string fragmentCode;
//         std::ifstream vShaderFile;
//         std::ifstream fShaderFile;

//         vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
//         fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

//         try {
//             vShaderFile.open(vertexPath);
//             fShaderFile.open(fragmentPath);
//             std::stringstream vShaderStream, fShaderStream;
//             vShaderStream << vShaderFile.rdbuf();
//             fShaderStream << fShaderFile.rdbuf();
//             vShaderFile.close();
//             fShaderFile.close();
//             vertexCode = vShaderStream.str();
//             fragmentCode = fShaderStream.str();
//         }
//         catch (std::ifstream::failure& e) {
//             std::cerr << "错误：读取 Shader 文件失败！请检查路径是否正确。" << std::endl;
//             std::cerr << "尝试读取的路径: " << vertexPath << " 和 " << fragmentPath << std::endl;
//         }

//         const char* vShaderCode = vertexCode.c_str();
//         const char* fShaderCode = fragmentCode.c_str();

//         // 2. 编译并链接
//         GLuint vertex, fragment;
        
//         vertex = glCreateShader(GL_VERTEX_SHADER);
//         glShaderSource(vertex, 1, &vShaderCode, NULL);
//         glCompileShader(vertex);
//         checkCompileErrors(vertex, "VERTEX");

//         fragment = glCreateShader(GL_FRAGMENT_SHADER);
//         glShaderSource(fragment, 1, &fShaderCode, NULL);
//         glCompileShader(fragment);
//         checkCompileErrors(fragment, "FRAGMENT");

//         ID = glCreateProgram();
//         glAttachShader(ID, vertex);
//         glAttachShader(ID, fragment);
//         glLinkProgram(ID);
//         checkCompileErrors(ID, "PROGRAM");

//         glDeleteShader(vertex);
//         glDeleteShader(fragment);
//     }

//     void use() { glUseProgram(ID); }

// private:
//     void checkCompileErrors(GLuint shader, std::string type) {
//         int success;
//         char infoLog[1024];
//         if (type != "PROGRAM") {
//             glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
//             if (!success) {
//                 glGetShaderInfoLog(shader, 1024, NULL, infoLog);
//                 std::cout << "Shader 编译错误 (" << type << "):\n" << infoLog << std::endl;
//             }
//         } else {
//             glGetProgramiv(shader, GL_LINK_STATUS, &success);
//             if (!success) {
//                 glGetProgramInfoLog(shader, 1024, NULL, infoLog);
//                 std::cout << "Shader 程序链接错误:\n" << infoLog << std::endl;
//             }
//         }
//     }
// };
// #endif

#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    GLuint ID;

    // 渲染着色器构造函数 (Vert + Frag)
    Shader(const char* vertexPath, const char* fragmentPath) {
        std::string vCode = readFile(vertexPath);
        std::string fCode = readFile(fragmentPath);
        GLuint v = compile(vCode.c_str(), GL_VERTEX_SHADER, "VERTEX");
        GLuint f = compile(fCode.c_str(), GL_FRAGMENT_SHADER, "FRAGMENT");
        ID = glCreateProgram();
        glAttachShader(ID, v);
        glAttachShader(ID, f);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        glDeleteShader(v); glDeleteShader(f);
    }

    // 计算着色器构造函数 (Compute Only)
    Shader(const char* computePath) {
        std::string cCode = readFile(computePath);
        GLuint c = compile(cCode.c_str(), GL_COMPUTE_SHADER, "COMPUTE");
        ID = glCreateProgram();
        glAttachShader(ID, c);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        glDeleteShader(c);
    }

    void use() { glUseProgram(ID); }

private:
    std::string readFile(const char* path) {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            file.open(path);
            std::stringstream stream;
            stream << file.rdbuf();
            return stream.str();
        } catch (...) {
            std::cerr << "File Read Error: " << path << std::endl;
            return "";
        }
    }

    GLuint compile(const char* code, GLenum type, std::string name) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &code, NULL);
        glCompileShader(s);
        checkCompileErrors(s, name);
        return s;
    }

    void checkCompileErrors(GLuint shader, std::string type) {
        int success; char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) { glGetShaderInfoLog(shader, 1024, NULL, infoLog); std::cout << type << " Error:\n" << infoLog << std::endl; }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) { glGetProgramInfoLog(shader, 1024, NULL, infoLog); std::cout << type << " Error:\n" << infoLog << std::endl; }
        }
    }
};
#endif