# Prototype-02
A naive image processing tool for data-set augmentation using multi-thread and OpenGL Shaders.
  
Performance on Intel Core-M 7y30 1.0-2.0GHz (2c4t), using 4 threads, with DVFS enabled.  
Input: 4000x4000 JPG  
Output: 512x512 JPG (9 output images for every input image)  
Speed: 35~60 output image/sec  
  
Input: 512x512 JPG  
Output: 256x256 JPG (9 output images for every input image)  
Speed: 200~350 output image/sec  

Dependencies:
- GLM
- glfw3, glew
- stb_image(_write)
