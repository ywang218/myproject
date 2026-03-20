#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> // 必须包含这个才能用 value_ptr

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
        if (vCode.empty() || fCode.empty()) return;

        GLuint v = compile(vCode.c_str(), GL_VERTEX_SHADER, "VERTEX");
        GLuint f = compile(fCode.c_str(), GL_FRAGMENT_SHADER, "FRAGMENT");
        
        ID = glCreateProgram();
        glAttachShader(ID, v);
        glAttachShader(ID, f);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        
        glDeleteShader(v); 
        glDeleteShader(f);
    }

    // 计算着色器构造函数 (Compute Only)
    Shader(const char* computePath) {
        std::string cCode = readFile(computePath);
        if (cCode.empty()) return;

        GLuint c = compile(cCode.c_str(), GL_COMPUTE_SHADER, "COMPUTE");
        ID = glCreateProgram();
        glAttachShader(ID, c);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        glDeleteShader(c);
    }

    void use() { glUseProgram(ID); }

    // --- 🚀 新增：像 Three.js 一样方便地设置 Uniforms ---
    
    void setBool(const std::string &name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    
    void setInt(const std::string &name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    
    void setFloat(const std::string &name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    // 对应 Three.js 的 Vector3
    void setVec3(const std::string &name, const glm::vec3 &value) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    // 对应 Three.js 的 Matrix4
    void setMat4(const std::string &name, const glm::mat4 &mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

private:
    std::string readFile(const char* path) {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            file.open(path);
            std::stringstream stream;
            stream << file.rdbuf();
            return stream.str();
        } catch (const std::exception& e) {
            std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << path << " | " << e.what() << std::endl;
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
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};
#endif

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

//     // 渲染着色器构造函数 (Vert + Frag)
//     Shader(const char* vertexPath, const char* fragmentPath) {
//         std::string vCode = readFile(vertexPath);
//         std::string fCode = readFile(fragmentPath);
//         GLuint v = compile(vCode.c_str(), GL_VERTEX_SHADER, "VERTEX");
//         GLuint f = compile(fCode.c_str(), GL_FRAGMENT_SHADER, "FRAGMENT");
//         ID = glCreateProgram();
//         glAttachShader(ID, v);
//         glAttachShader(ID, f);
//         glLinkProgram(ID);
//         checkCompileErrors(ID, "PROGRAM");
//         glDeleteShader(v); glDeleteShader(f);
//     }

//     // 计算着色器构造函数 (Compute Only)
//     Shader(const char* computePath) {
//         std::string cCode = readFile(computePath);
//         GLuint c = compile(cCode.c_str(), GL_COMPUTE_SHADER, "COMPUTE");
//         ID = glCreateProgram();
//         glAttachShader(ID, c);
//         glLinkProgram(ID);
//         checkCompileErrors(ID, "PROGRAM");
//         glDeleteShader(c);
//     }

//     void use() { glUseProgram(ID); }

// private:
//     std::string readFile(const char* path) {
//         std::ifstream file;
//         file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
//         try {
//             file.open(path);
//             std::stringstream stream;
//             stream << file.rdbuf();
//             return stream.str();
//         } catch (...) {
//             std::cerr << "File Read Error: " << path << std::endl;
//             return "";
//         }
//     }

//     GLuint compile(const char* code, GLenum type, std::string name) {
//         GLuint s = glCreateShader(type);
//         glShaderSource(s, 1, &code, NULL);
//         glCompileShader(s);
//         checkCompileErrors(s, name);
//         return s;
//     }

//     void checkCompileErrors(GLuint shader, std::string type) {
//         int success; char infoLog[1024];
//         if (type != "PROGRAM") {
//             glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
//             if (!success) { glGetShaderInfoLog(shader, 1024, NULL, infoLog); std::cout << type << " Error:\n" << infoLog << std::endl; }
//         } else {
//             glGetProgramiv(shader, GL_LINK_STATUS, &success);
//             if (!success) { glGetProgramInfoLog(shader, 1024, NULL, infoLog); std::cout << type << " Error:\n" << infoLog << std::endl; }
//         }
//     }
// };
// #endif


