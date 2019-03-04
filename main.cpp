#include <iostream>
#include <sstream>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <pthread.h>
#include <unistd.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

const int TRANSMAT_NUMBER = 9;
const float TILT_ANGLE = 45.0f;

const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 256;

// Multi-Thread Support
const int SECTOR_SIZE = SCREEN_WIDTH * SCREEN_HEIGHT * 3;	//RGB 3bytes
const int THREAD_NUMBER = 8;

int g_count = 0;
int arg_presentnum = 1;

GLubyte* pixel_data;	// Pixel Memory Buffer
pthread_t tids[THREAD_NUMBER];
pthread_mutex_t sector_mutex[THREAD_NUMBER];

void* stb_thread_entry(void* arg)
{
	int count_tmp = (long int)arg;
	
	stringstream filename_ss;
	string filename;
	
	filename_ss << "./output/" << arg_presentnum << "_" << count_tmp << ".jpg";
	filename_ss >> filename;
	
	stbi_write_jpg(	filename.c_str(),
					SCREEN_WIDTH,
					SCREEN_HEIGHT,
					3,
					pixel_data + SECTOR_SIZE * (count_tmp % THREAD_NUMBER),
					90);
	/*
	stbi_write_bmp(filename.c_str(),
					SCREEN_WIDTH,
					SCREEN_HEIGHT,
					3,
					pixel_data + SECTOR_SIZE * (count_tmp % THREAD_NUMBER));
	*/
	
	pthread_mutex_unlock(&sector_mutex[count_tmp % THREAD_NUMBER]);
	pthread_exit(NULL);
}

int main(int argc, char** argv)
{
    if(argv[1] == NULL)
    {
        printf("usage: %s {TARGETFILES}\n",argv[0]);
        return -1;
    }
    
    //OpenGL Initialization ----------------------------------------------------
	if(!glfwInit())
	{
		std::cout<<"Error:GLFW init failed."<<std::endl;
		return -1;
	}
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	
	// GLFW Offscreen Rendering Configuration-----------------------------------
	glfwWindowHint(GLFW_VISIBLE, 0);
	
	GLFWwindow* window = glfwCreateWindow(	SCREEN_WIDTH,
											SCREEN_HEIGHT,
											"Project 1725 / OpenGL 4.5",
											NULL,
											NULL);
	if(!window)
	{
		glfwTerminate();
		std::cout<<"Error:Failed to Create Window."<<std::endl;
		return -1;
	}
	
	glfwMakeContextCurrent(window);
	
	//Vsync
	glfwSwapInterval(0);
	
	glewExperimental = GL_TRUE;
	
	//Causing OpenGL 1280 Error on Intel Core-M 7y30, Ubuntu 16.04 LTS
    GLenum err = glewInit();
    
    if (GLEW_OK != err)
    {
    	std::cout<<"GLEW_ERROR"<<std::endl;
    	std::cout<<glewGetErrorString(err)<<std::endl;
        return -1;
    }
	
	// OpenGL Configuration-----------------------------------------------------
    //glEnable(GL_DEPTH_TEST);
    //glEnable(GL_BLEND);
    //glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	// Shader
	Shader shader1("GLSL/Vertex.glsl","GLSL/Fragment.glsl");
    shader1.use();
    
    //model transformation
    glm::mat4 trans_mod = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0,0.0,0.0));
    //projection transformaion
    glm::mat4 trans_proj = glm::perspective(glm::radians(60.0f),(float)SCREEN_WIDTH/(float)SCREEN_HEIGHT,0.1f,100.0f);
    //view trasnformation
    glm::mat4 trans_view = glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,0.0f,-1.4f));
	//send config to shader
	shader1.setMat4("trans_mod",trans_mod);
	shader1.setMat4("trans_proj",trans_proj);
	shader1.setMat4("trans_view",trans_view);
	
    
    // Contents-----------------------------------------------------------------
	float vertex[]=    //SPFP VERTEX
	{
		/*3	 1
		  0	 2*/
		//front X,Y,Z  		U,V
		-0.5f,-0.5f,0.0f,	0.0f,1.0f,//0
		0.5f,0.5f,0.0f,		1.0f,0.0f,//1
		0.5f,-0.5f,0.0f,	1.0f,1.0f,//2
		-0.5f,0.5f,0.0f,	0.0f,0.0f,//3
	};
	
	unsigned int indicies[]=
	{
		0,1,3,
		0,2,1
	};
    
    // VAO, VBO, EBO -----------------------------------------------------------
    GLuint VAO,VBO,EBO;
    
    glCreateVertexArrays(1,&VAO);
    glCreateBuffers(1,&VBO);
    glCreateBuffers(1,&EBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
    
    glNamedBufferStorage(VBO, sizeof(vertex), vertex, 0);
    glNamedBufferStorage(EBO, sizeof(indicies), indicies, 0);
    
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    
    //FBO-----------------------------------------------------------------------
    GLuint FBO;
    glGenFramebuffers(1,&FBO);
    glBindFramebuffer(GL_FRAMEBUFFER,FBO);
    
    //RGBA Buffer
    GLuint texture_fbo;
	glGenTextures(1, &texture_fbo);
	glBindTexture(GL_TEXTURE_2D, texture_fbo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_WIDTH, SCREEN_HEIGHT, 0,
				GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_MIRRORED_REPEAT);
	
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_fbo,0);
    
	//RBO
	GLuint RBO;
	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_WIDTH, SCREEN_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER, RBO);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    
    
    // Resource Allocating------------------------------------------------------
    pixel_data = (GLubyte*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 3 * THREAD_NUMBER);
	for(int i = 0; i != THREAD_NUMBER; i++)
		pthread_mutex_init(&sector_mutex[i], NULL);
    
    //Model Transformation Matrix Adjustments
    glm::mat4 trans_mod_array[TRANSMAT_NUMBER];
    for(int i = 0; i != TRANSMAT_NUMBER; i++)
    {
    	trans_mod_array[i]= glm::rotate(glm::mat4(1.0f),						//unit matrix
    									glm::radians(((i%3)-1) * TILT_ANGLE),	//angle
    									glm::vec3(0.0,1.0,0.0));				//normal
    }
    for(int i = 0; i != TRANSMAT_NUMBER; i++)
    {
    	trans_mod_array[i]= glm::rotate(trans_mod_array[i],						//unit matrix
    									glm::radians(((i/3)-1) * TILT_ANGLE),	//angle
    									glm::vec3(1.0,0.0,0.0));				//normal
    }
    
	// Texture------------------------------------------------------------------
	GLuint texture;
    glGenTextures(1,&texture);
    glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_MIRRORED_REPEAT);
    
    // Main Loop
    while(arg_presentnum < argc)
    {
    	// STBI Image Loading-------------------------------------------------------
    	int width, height, nrchannels;
    	unsigned char* data = stbi_load(argv[arg_presentnum],&width,&height,&nrchannels,0);
		if(!data)
    		std::cout<<"FAILED TO LOAD TEXTURE !"<<std::endl;
    	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,data);
    	glGenerateMipmap(GL_TEXTURE_2D);
    	stbi_image_free(data);
    	
    	g_count = 0;
    	// Render Loop--------------------------------------------------------------
    	while (!glfwWindowShouldClose(window) && g_count != TRANSMAT_NUMBER)
    	{
    		shader1.setMat4("trans_mod",trans_mod_array[g_count]);
    		// Lock The FBO to avoid tearing. (because no Vsync)
    		pthread_mutex_lock(&sector_mutex[g_count % THREAD_NUMBER]);
    		    	
    		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
    		glDrawElements(GL_TRIANGLES, sizeof(indicies), GL_UNSIGNED_INT, 0);
    	
    		// Export Result to RAM
    		glReadPixels(	0,
    						0,
    						SCREEN_WIDTH,
    						SCREEN_HEIGHT,
    						GL_RGB,
    						GL_UNSIGNED_BYTE,
    						pixel_data + (g_count % THREAD_NUMBER) * SECTOR_SIZE);
    		
    		// Start threads
    		pthread_create(	&tids[g_count % THREAD_NUMBER],
    						NULL,
    						stb_thread_entry,
    						(void*)(long int)g_count);
        
       		// Swap FrameBuffer
        	glfwSwapBuffers(window);
        	glfwPollEvents();
        
    		g_count++;
    	}
    	for(int i = 0; i != THREAD_NUMBER; i++)
			pthread_join(tids[i], NULL);
		arg_presentnum++;
	}
    
    // Resource Deallocating----------------------------------------------------
	for(int i = 0; i != THREAD_NUMBER; i++)
	{
		pthread_mutex_destroy(&sector_mutex[i]);
	}
	
	free(pixel_data);
	
	
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
