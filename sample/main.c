/* main.c
*
* Copyright (C) 2017 Jules Blok
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "lodepng.h"
#include "linmath.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static const struct
{
    float x, y, z, w;
    float u, v, s, t;
} vertices[] =
{
    { -1.f, -1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f },
    { -1.f,  1.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f },
    {  1.f,  1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f },
    {  1.f, -1.f, 0.f, 1.f, 1.f, 1.f, 0.f, 0.f }
};

static const uint8_t indices[] = { 0, 1, 2, 0, 2, 3 };

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
	assert(FALSE);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void read_file(const char* filename, char** data)
{
	uint8_t* buffer;
	FILE* file;
	long fsize;

	file = fopen(filename, "rb");
	if (file == NULL)
		exit(EXIT_FAILURE);

	fseek(file, 0, SEEK_END);
	fsize = ftell(file);
	fseek(file, 0, SEEK_SET);

	buffer = malloc(fsize + 1);
	fread(buffer, fsize, 1, file);
	buffer[fsize] = '\0';
	fclose(file);
	if (data) *data = buffer;
}

static GLuint load_texture(GLenum stage, uint32_t* width, uint32_t* height, const char* filename)
{
	uint8_t* image;
	uint32_t w, h, error;
	GLuint texture;

	error = lodepng_decode32_file(&image, &w, &h, filename);
	if (error)
	{
		error_callback(error, lodepng_error_text(error));
		exit(EXIT_FAILURE);
	}

	glGenTextures(1, &texture);
	glActiveTexture(stage);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	free(image);

	if (width) *width = w;
	if (height) *height = h;
	return texture;
}

static GLuint compile_shader(GLenum stage, const GLchar* source)
{
	GLchar* error_log;
	GLint compiled, length;
	GLuint shader;
	const GLchar* sources[2] = { "#version 130\n", source };

	if (stage == GL_VERTEX_SHADER)
		sources[0] = "#version 130\n#define VERTEX\n";

	if (stage == GL_FRAGMENT_SHADER)
		sources[0] = "#version 130\n#define FRAGMENT\n";

	shader = glCreateShader(stage);
	glShaderSource(shader, 2, sources, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (compiled == GL_FALSE)
	{
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		error_log = malloc(length);
		glGetShaderInfoLog(shader, length, &length, error_log);
		error_callback(GL_INVALID_OPERATION, error_log);
		free(error_log);
	}

	return shader;
}

static GLuint link_program(GLuint vertex_shader, GLuint fragment_shader)
{
	GLchar* error_log;
	GLint compiled, length;
	GLuint program;

	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, (int *)&compiled);
	if (compiled == GL_FALSE)
	{
		glGetShaderiv(program, GL_INFO_LOG_LENGTH, &length);
		error_log = malloc(length);
		glGetProgramInfoLog(program, length, &length, error_log);
		error_callback(GL_INVALID_OPERATION, error_log);
		free(error_log);
	}
	return program;
}

int main(int argc, const char* argv[])
{
    GLFWwindow* window;
    GLuint vertex_buffer, vertex_shader, fragment_shader, program, texture, lut;
	GLint mvp_location, samp_location, lut_location, vpos_location, vtex_location, tsize_location;
	uint32_t width, height, scale;
	char* shader;
	mat4x4 mvp;

	if (argc < 4)
	{
		printf("usage %s: scale shader lut image\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	scale = atoi(argv[1]);
	if (scale <= 0)
		exit(EXIT_FAILURE);

	read_file(argv[2], &shader);

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(640, 480, "HQx Sample", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

	texture = load_texture(GL_TEXTURE0, &width, &height, argv[4]);
	lut = load_texture(GL_TEXTURE1, NULL, NULL, argv[3]);
	glfwSetWindowSize(window, width * scale, height * scale);

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	vertex_shader = compile_shader(GL_VERTEX_SHADER, shader);
	fragment_shader = compile_shader(GL_FRAGMENT_SHADER, shader);
	program = link_program(vertex_shader, fragment_shader);
	glUseProgram(program);

	mvp_location = glGetUniformLocation(program, "MVPMatrix");
	samp_location = glGetUniformLocation(program, "Texture");
	lut_location = glGetUniformLocation(program, "LUT");
	tsize_location = glGetUniformLocation(program, "TextureSize");
    vpos_location = glGetAttribLocation(program, "VertexCoord");
	vtex_location = glGetAttribLocation(program, "TexCoord");

	mat4x4_identity(mvp);
	glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*)mvp);

	glUniform1i(samp_location, 0);
	glUniform1i(lut_location, 1);
	glUniform2f(tsize_location, (float)width, (float)height);
    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location, 4, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void*) 0);
    glEnableVertexAttribArray(vtex_location);
    glVertexAttribPointer(vtex_location, 4, GL_FLOAT, GL_FALSE,
                          sizeof(vertices[0]), (void*) (sizeof(float) * 4));

    while (!glfwWindowShouldClose(window))
    {
        int fwidth, fheight;
        glfwGetFramebufferSize(window, &fwidth, &fheight);

        glViewport(0, 0, fwidth, fheight);
        glClear(GL_COLOR_BUFFER_BIT);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
