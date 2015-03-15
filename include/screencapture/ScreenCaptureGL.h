/* -*-c++-*- */
/*

  -------------------------------------------------------------------------

  Copyright (C) 2015 roxlu <info#AT#roxlu.com> All Rights Reserved.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  
  -------------------------------------------------------------------------


  OpenGL Screen Capture
  ======================
  
  The `ScreenCaptureGL` class can be used when a GL context is available
  and you want to draw the captured screen. Because every OS and game engine
  has it's own way to include the GL headers you need to include this header
  only after you've include the necessary GL headers (and so all GL functions 
  that we need are available). Then, in one file you need to use 
  `#defined SCREEN_CAPTURE_IMPLEMENTATION` to include the implementation of 
  this class (also after including the GL headers.).

  How to include
  ---------------
  In every file where you want to use the `ScreenCaptureGL` class you can use: 
 
         #include <screencapture/ScreenCaptureGL.h>

   And in one .cpp file you need to do:
   
         #define SCREEN_CAPUTRE_IMPLEMENTATION
         #include <screencapture/ScreenCaptureGL.h>


  Usage 
  -----
  See src/test_opengl.cpp

*/
/* --------------------------------------------------------------------------- */
/*                               H E A D E R                                   */ 
/* --------------------------------------------------------------------------- */
#ifndef SCREENCAPTURE_OPENGL_H
#define SCREENCAPTURE_OPENGL_H

#include <assert.h>
#include <screencapture/ScreenCapture.h>

#if defined(_WIN32)
   #define WIN32_LEAN_AND_MEAN
   #include <windows.h>
#elif defined(__linux) or defined(__APPLE__)
   #include <pthread.h>
#endif

namespace sc {
  
  /* --------------------------------------------------------------------------- */
  /* Shaders                                                                     */
  /* --------------------------------------------------------------------------- */
  
  static const char* SCREENCAPTURE_GL_VS = ""
    "#version 330\n"
    "uniform mat4 u_pm;"
    "uniform mat4 u_vm;"
    "uniform float u_texcoords[8]; "
    ""
#if 0    
    "const vec2 pos[4] = vec2[4]("
    "  vec2(0.0,  1.0), "
    "  vec2(0.0,  0.0), "
    "  vec2(1.0,  1.0), "
    "  vec2(1.0,  0.0)  "
    ");"
#else
    "const vec2 pos[4] = vec2[4]("
    "  vec2(-1.0,  1.0), "
    "  vec2(-1.0,  -1.0), "
    "  vec2(1.0,  1.0), "
    "  vec2(1.0,  -1.0)  "
    ");"
#endif
    ""
    "out vec2 v_tex; "
    ""
    "void main() { "
#if 0    
    "  gl_Position = u_pm * u_vm * vec4(pos[gl_VertexID], 0.0, 1.0);"
    "  gl_Position.z = 1.0;"
#else
    "  gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);"
#endif
    "  v_tex = vec2(u_texcoords[gl_VertexID * 2], u_texcoords[gl_VertexID * 2 + 1]);"
    "}"
    "";

  static const char* SCREENCAPTURE_GL_FS_BGRA = ""
    "#version 330\n"
    "uniform sampler2D u_tex;"
    ""
    "in vec2 v_tex;"
    "layout( location = 0 ) out vec4 fragcolor; "
    ""
    "void main() {"
    "  vec4 tc = texture(u_tex, v_tex);"
    "  fragcolor.rgb = tc.bgr;"
    "  fragcolor.a = 1.0f;"
    "}"
    "";
  
  /* --------------------------------------------------------------------------- */
  /* Cross platform mutex.                                                       */
  /* --------------------------------------------------------------------------- */

#if defined(_WIN32)
  struct ScreenMutex {
    CRITICAL_SECTION handle;
  };
#elif defined(__linux) or defined(__APPLE__)
  struct ScreenMutex {
    pthread_mutex_t handle;
  };
#endif
  
  int sc_gl_create_mutex(ScreenMutex& m);
  int sc_gl_destroy_mutex(ScreenMutex& m);
  int sc_gl_lock_mutex(ScreenMutex& m);
  int sc_gl_unlock_mutex(ScreenMutex& m);
  
  /* --------------------------------------------------------------------------- */
  /* OpenGL ScreenCapture drawer                                                 */
  /* --------------------------------------------------------------------------- */
  
  class ScreenCaptureGL {
  public:
    ScreenCaptureGL();
    ~ScreenCaptureGL();

    int init();                                                                  /* Initializes the screencapturer driver. Should be used as constructor.*/
    int shutdown();                                                              /* Shutdown the capturer, removes allocated GL textures and memory for the received frames. */
    int configure(Settings settings);                                            /* Configure the capturer, see Types.h for the available settings. */
    int start();                                                                 /* Start capturing the screen. */
    int stop();                                                                  /* Stop capturing the screen. */

    void update();                                                               /* Update the texture; must be called before you want to draw. */ 
    void draw();                                                                 /* Draw the captured screen using at 0, 0 using the provided output_width and output_height of the settings youpassed into configure.*/
    void draw(float x, float y, float w, float h);                               /* Draw the captured screen at the given location. */
    int flip(bool horizontal, bool vertical);                                    /* Flip the screen horizontally or vertically according to Photoshops flip feature. */

    int setProjectionMatrix(float* projection); 
    
  private:
    int setupGraphics();                                                         /* Used internally to create the shaders when they're not yet created, calls `setupShaders()` and `setupTextures()`. */
    int setupShaders();                                                          /* Creates the shader when it's not yet created; all `ScreenCaptureGL instances shared the same shader instance. */
    int setupTextures();                                                         /* Create the texture(s) that are needed to render the captured screen in the set pixel format. */

  public:
    static GLuint prog;                                                          /* The shader program that renders the captured screen (shared among all `ScreenCapture` instances). */
    static GLuint vert;                                                          /* The vertex shader. Shared among all instances of `ScreenCapture` */
    static GLuint frag;                                                          /* The fragment shader. Shared among all instances of `ScreenCapture` */
    static GLuint vao;                                                           /* The vao we use for attributeless rendering. */
    static GLint u_pm;                                                           /* Projection matrix uniform location. */
    static GLint u_vm;                                                           /* View matrix uniform location. */
    static GLint u_texcoords;                                                    /* Texcoord uniform location; `flip()` uploads the correct texture coordinates. */
    static GLint u_tex;                                                          /* Texture uniform location. */ 
    ScreenMutex mutex;                                                           /* It's possible (and probably will be) that the received pixel buffers are passed to the frame callback from another thread so we need to sync access to it. */
    ScreenCapture cap;                                                           /* The actual capturer */
    Settings settings;                                                           /* The settings you passed into `configure()`. */
    uint8_t* pixels;                                                             /* The pixels we received in the frame callback. */
    bool has_new_frame;                                                          /* Is set to true when in the frame callback. When true, we will update the texture data. */
    GLuint tex0;                                                                 /* First plane, or the only tex when using GL_BGRA */
    GLuint tex1;                                                                 /* Second plane, only used when we receive planar data. */  
    float pm[16];                                                                /* The projection matrix. */
    float vm[16];                                                                /* The view matrix. */
  };

  /* --------------------------------------------------------------------------- */
  
  inline int ScreenCaptureGL::start() {
      return cap.start();
  }

  inline int ScreenCaptureGL::stop() {
    return cap.stop();
  }

  inline int ScreenCaptureGL::setProjectionMatrix(float* projection) {

#if !defined(NDEBUG)
    if (0 == prog) {
      printf("Error: can only set the projection matrix of the ScreenCaptureGL after it's initialized and configured.\n");
      return -1;
    }
#endif    
    
    if (NULL == projection) {
      printf("Error: trying to set the projection matrix, but you've passed a NULL pointer.\n");
      return -2;
    }

    memcpy(pm, projection, sizeof(pm));

    glUseProgram(prog);
    glUniformMatrix4fv(u_pm, 1, GL_FALSE, pm);

    return 0;
  }

} /* namespace sc */
#endif


/* --------------------------------------------------------------------------- */
/*                       I M P L E M E N T A T I O N                           */ 
/* --------------------------------------------------------------------------- */
#if defined(SCREEN_CAPTURE_IMPLEMENTATION)

namespace sc {

  static void create_ortho_matrix(float l, float r, float b, float t, float n, float f, float* m);
  static void create_identity_matrix(float* m);
  static void create_translation_matrix(float x, float y, float z, float* m);

  /* See: https://gist.github.com/roxlu/6152fccfdd0446533e1b for latest version. */
  /* --------------------------------------------------------------------------- */
  /* Embeddable OpenGL wrappers.                                                 */
  /* --------------------------------------------------------------------------- */
  static int create_shader(GLuint* out, GLenum type, const char* src);           /* Create a shader, returns 0 on success, < 0 on error. e.g. create_shader(&vert, GL_VERTEX_SHADER, DRAWER_VS); */
  static int create_program(GLuint* out, GLuint vert, GLuint frag, int link);    /* Create a program from the given vertex and fragment shader. returns 0 on success, < 0 on error. e.g. create_program(&prog, vert, frag, 1); */
  static int print_shader_compile_info(GLuint shader);                           /* Prints the compile info of a shader. returns 0 when shader compiled, < 0 on error. */
  static int print_program_link_info(GLuint prog);                               /* Prints the program link info. returns 0 on success, < 0 on error. */
  /* --------------------------------------------------------------------------- */

  static void sc_gl_frame_callback(PixelBuffer& buffer);
  
  /* --------------------------------------------------------------------------- */
  
  GLuint ScreenCaptureGL::prog = 0;
  GLuint ScreenCaptureGL::vert = 0;
  GLuint ScreenCaptureGL::frag = 0;
  GLuint ScreenCaptureGL::vao = 0;
  GLint ScreenCaptureGL::u_pm = -1;
  GLint ScreenCaptureGL::u_vm = -1;
  GLint ScreenCaptureGL::u_texcoords = -1;
  GLint ScreenCaptureGL::u_tex = -1;

  /* --------------------------------------------------------------------------- */

  ScreenCaptureGL::ScreenCaptureGL()
    :cap(sc_gl_frame_callback, this)
    ,pixels(NULL)
    ,has_new_frame(false)
    ,tex0(0)
    ,tex1(0)
  {
    memset(pm, 0x00, sizeof(pm));
    memset(vm, 0x00, sizeof(vm));
  }

  ScreenCaptureGL::~ScreenCaptureGL() {
    
    shutdown();
  }

  int ScreenCaptureGL::init() {

    if (0 != sc_gl_create_mutex(mutex)) {
      return -1;
    }

    if (0 != cap.init()) {
      return -1;
    }
    
    return 0;
  }

  int ScreenCaptureGL::shutdown() {

    int r = 0;
    
    if (0 != sc_gl_destroy_mutex(mutex)) {
      r -= 1;
    }

    if (0 != cap.shutdown()) {
      r -= 2;
    }

    if (NULL != pixels) {
      delete[] pixels;
      pixels = NULL;
    }

    if (0 != tex0) {
      glDeleteTextures(1, &tex0);
      tex0 = 0;
    }

    if (0 != tex1) {
      glDeleteTextures(1, &tex1);
      tex1 = 0;
    }

    has_new_frame = false;

    return r;
  }

  int ScreenCaptureGL::configure(Settings cfg) {

    size_t nbytes = 0;

    settings = cfg;

    if (SC_BGRA != cfg.pixel_format) {
      printf("Error: currently we only support SC_BGRA in ScreenCaptureGL.\n");
      return -1;
    }
    
    if (0 != cap.configure(cfg)) {
      return -2;
    }

    if (NULL != pixels) {
      delete[] pixels;
      pixels = NULL;
    }

    if (SC_BGRA == cfg.pixel_format) {
      nbytes = cfg.output_width * cfg.output_height * 4;
      pixels = new uint8_t[nbytes];
      memset(pixels, 0x00, nbytes);
    }
    else {
      printf("Error: unsupported pixel format in ScreenCaptureGL::configure: %s\n", screencapture_pixelformat_to_string(cfg.pixel_format).c_str());
      return -3;
    }
    
    if (0 != setupGraphics()) {
      return -4;
    }
    
    return 0;
  }
  
  int ScreenCaptureGL::setupGraphics() {

    if (0 == prog) {
      if (0 != setupShaders()) {
        return -1;
      }
    }

    if (0 == tex0) {
      if (0 != setupTextures()) {
        return -2;
      }
    }

    if (0 == vao) {
      glGenVertexArrays(1, &vao);
    }

    create_identity_matrix(vm);
    
    if (0 != flip(false, true)) {
      /* @todo - we should remove all GL objects here. */
      return -3;
    }
    
    return 0;
  }

  int ScreenCaptureGL::setupShaders() {

    GLint vp[4] = { 0.0f };

    if (0 != prog) {
      printf("Error: trying to setup shaders but the shader is already created.\n");
      return -1;
    }

    if (SC_NONE == settings.pixel_format) {
      printf("Error: cannot setup screencapture shaders; pixel format not set in settings.\n");
      return -2;
    }

    if (0 != create_shader(&vert, GL_VERTEX_SHADER, SCREENCAPTURE_GL_VS)) {
      printf("Error: failed to create the screencapture vertex shader.\n");
      return -2;
    }

    if (SC_BGRA == settings.pixel_format) {
      if (0 != create_shader(&frag, GL_FRAGMENT_SHADER, SCREENCAPTURE_GL_FS_BGRA)) {
        printf("Error: failed to create the screencapture fragment shader.\n");
        return -3;
      }
    }
    else {
      printf("Error: failed to create the shader; no supported shader found.\n");
      glDeleteShader(vert);
      vert = -1;
      return -4;
    }

    if (0 != create_program(&prog, vert, frag, 1)) {
      printf("Error: failed to create the screencapture program.\n");
      glDeleteShader(vert);
      glDeleteShader(frag);
      vert = -1;
      frag = -1;
      return -5;
    }

    glUseProgram(prog);

    u_pm = glGetUniformLocation(prog, "u_pm");
    u_vm = glGetUniformLocation(prog, "u_vm");
    u_texcoords = glGetUniformLocation(prog, "u_texcoords");
    u_tex = glGetUniformLocation(prog, "u_tex");

    assert(-1 != u_pm);
    assert(-1 != u_vm);
    assert(-1 != u_texcoords);
    assert(-1 != u_tex);

    glGetIntegerv(GL_VIEWPORT, vp);
    create_ortho_matrix(0.0f, vp[2], vp[3], 0.0f, 0.0f, 100.0f, pm);

    glUniformMatrix4fv(u_pm, 1, GL_FALSE, pm);
    glUniform1i(u_tex, 0);
    
    return 0;
  }

  int ScreenCaptureGL::setupTextures() {

    if (0 > settings.output_width) {
      printf("Error: cannot setup the texture for screencapture, output width is invalid: %d\n", settings.output_width);
      return -1;
    }

    if (0 > settings.output_height) {
      printf("Error: cannot setup the texture for screencapture, output height is invalid: %d\n", settings.output_height);
      return -2;
    }

    if (SC_BGRA == settings.pixel_format) {

      if (0 != tex0) {
        printf("Error: trying to setup the textures, but it seems that the texture is already created.\n");
        return -3;
      }

      glGenTextures(1, &tex0);
      glBindTexture(GL_TEXTURE_2D, tex0);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, settings.output_width, settings.output_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else {
      printf("Error: we didn't create textures for the ScreenCaptureGL because we need to add support for the pixel format: %s\n", screencapture_pixelformat_to_string(settings.pixel_format).c_str());
      return -4;
    }
      
    return 0;
  }

  void ScreenCaptureGL::update() {
    
    sc_gl_lock_mutex(mutex);
    {
      if (true == has_new_frame) {
        glBindTexture(GL_TEXTURE_2D, tex0);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, settings.output_width,  settings.output_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        has_new_frame = false;
      }
    }
    sc_gl_unlock_mutex(mutex);
  }

  void ScreenCaptureGL::draw() {
    draw(0.0f, 0.0f, settings.output_width, settings.output_height);
  }

  void ScreenCaptureGL::draw(float x, float y, float w, float h) {
    
#if !defined(NDEBUG)
    if (0 == prog) {
      printf("Error: cannot draw because the shader program hasn't been created.\n");
      return;
    }
#endif
    
    /* Based on the pixelf format bind the correct textures. */
    if (SC_BGRA == settings.pixel_format) {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, tex0);
    }
    else {
      printf("Error: cannot bind texture for the current pixel format. %s\n", screencapture_pixelformat_to_string(settings.pixel_format).c_str());
      return;
    }
   
    /* Update the view matrix. */
    vm[0] = w;
    vm[5] = h;
    vm[12] = x;
    vm[13] = y;

    /* And draw. */
    glUseProgram(prog);
    glBindVertexArray(vao);
    glUniformMatrix4fv(u_vm, 1, GL_FALSE, vm);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
  }

  int ScreenCaptureGL::flip(bool horizontal, bool vertical) {

    int dx = 0;
    float* texcoords = NULL;
    float tex_normal[] =     { 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0 } ;
    float tex_vert[] =       { 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0 } ;
    float tex_hori[] =       { 1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0 } ;
    float tex_verthori[] =   { 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 } ;

    if (0 == prog) {
      printf("Error: You're trying to flip the capture device but we're not yet setup.\n");
      return -1;
    }

    if (-1 == u_texcoords) {
      printf("Error: the u_texcoords uniform location hasn't been set.\n");
      return -2;
    }

    if (false == horizontal && false == vertical) {
      texcoords = tex_normal;
    }
    else if(false == horizontal && true == vertical) {
      texcoords = tex_vert;
    }
    else if(true == horizontal && false == vertical) {
      texcoords = tex_hori;
    }
    else if (true == horizontal && true == vertical) {
      texcoords = tex_verthori;
    }

    if (NULL == texcoords) {
      printf("Error: texcoords == NULL in %s\n", __FUNCTION__);
      return -2;
    }

    glUseProgram(prog);
    glUniform1fv(u_texcoords, 8, texcoords);

    return 0;
  }

  /* ------------------------------------------------------------------------*/
  /* Embeddable Math           .                                             */
  /* ------------------------------------------------------------------------*/
  
  static void create_ortho_matrix(float l, float r, float b, float t, float n, float f, float* m) {

    m[1]  = 0.0f;
    m[2]  = 0.0f;
    m[3]  = 0.0f;
    m[4]  = 0.0f;
    m[6]  = 0.0f;
    m[7]  = 0.0f;
    m[8]  = 0.0f;
    m[9]  = 0.0f;
    m[11] = 0.0f;
    m[15] = 1.0f;
    
    float rml = r - l;
    float fmn = f - n;
    float tmb = t - b;
    
    m[0]  = 2.0f / rml;
    m[5]  = 2.0f / tmb;
    m[10] = -2.0f / fmn;
    m[12] = -(r + l) / rml;
    m[13] = -(t + b) / tmb;
    m[14] = -(f + n) / fmn;
  }

  static void create_translation_matrix(float x, float y, float z, float* m) {
    m[0] = 1.0f;     m[4] = 0.0f;     m[8]  = 0.0f;    m[12] = x;
    m[1] = 0.0f;     m[5] = 1.0f;     m[9]  = 0.0f;    m[13] = y;
    m[2] = 0.0f;     m[6] = 0.0f;     m[10] = 1.0f;    m[14] = z;
    m[3] = 0.0f;     m[7] = 0.0f;     m[11] = 0.0f;    m[15] = 1.0f;
  }

  static void create_identity_matrix(float* m) {
    m[0] = 1.0f;     m[4] = 0.0f;     m[8]  = 0.0f;    m[12] = 0.0f;
    m[1] = 0.0f;     m[5] = 1.0f;     m[9]  = 0.0f;    m[13] = 0.0f;
    m[2] = 0.0f;     m[6] = 0.0f;     m[10] = 1.0f;    m[14] = 0.0f;
    m[3] = 0.0f;     m[7] = 0.0f;     m[11] = 0.0f;    m[15] = 1.0f;
  }

  /* ------------------------------------------------------------------------*/
  /* Embeddable OpenGL wrappers.                                             */
  /* ------------------------------------------------------------------------*/

  static int create_shader(GLuint* out, GLenum type, const char* src) {

    *out = glCreateShader(type);
    glShaderSource(*out, 1, &src, NULL);
    glCompileShader(*out);

    if (0 != print_shader_compile_info(*out)) {
      *out = 0;
      return -1;
    }

    return 0;
  }

  /* create a program, store the result in *out. when link == 1 we link too. returns -1 on error, otherwise 0 */
  static int create_program(GLuint* out, GLuint vert, GLuint frag, int link) {
    *out = glCreateProgram();
    glAttachShader(*out, vert);
    glAttachShader(*out, frag);

    if(1 == link) {
      glLinkProgram(*out);
      if (0 != print_program_link_info(*out)) {
        *out = 0;
        return -1;
      }
    }

    return 0;
  }

  /* checks + prints program link info. returns 0 when linking didn't result in an error, on link erorr < 0 */
  static int print_program_link_info(GLuint prog) {
    GLint status = 0;
    GLint count = 0;
    GLchar* error = NULL;
    GLsizei nchars = 0;

    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if(status) {
      return 0;
    }

    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &count);
    if(count <= 0) {
      return 0;
    }

    error = (GLchar*)malloc(count);
    glGetProgramInfoLog(prog, count, &nchars, error);
    if (nchars <= 0) {
      free(error);
      error = NULL;
      return -1;
    }

    printf("\nPROGRAM LINK ERROR");
    printf("\n--------------------------------------------------------\n");
    printf("%s", error);
    printf("--------------------------------------------------------\n\n");

    free(error);
    error = NULL;
    return -1;
  }

  /* checks the compile info, if it didn't compile we return < 0, otherwise 0 */
  static int print_shader_compile_info(GLuint shader) {

    GLint status = 0;
    GLint count = 0;
    GLchar* error = NULL;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status) {
      return 0;
    }

    error = (GLchar*) malloc(count);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &count);
    if(count <= 0) {
      free(error);
      error = NULL;
      return 0;
    }

    glGetShaderInfoLog(shader, count, NULL, error);
    printf("\nSHADER COMPILE ERROR");
    printf("\n--------------------------------------------------------\n");
    printf("%s", error);
    printf("--------------------------------------------------------\n\n");

    free(error);
    error = NULL;
    return -1;
  }

  /* --------------------------------------------------------------------------- */
  /* Cross platform mutex.                                                       */
  /* --------------------------------------------------------------------------- */

#if defined(_WIN32)

  int sc_gl_create_mutex(ScreenMutex& m) {
    InitializeCriticalSection(&m.handle);
    return 0;
  }

  int sc_gl_lock_mutex(ScreenMutex& m) {
    EnterCriticalSection(&m.handle);
    return 0;
  }

  int sc_gl_unlock_mutex(ScreenMutex& m) {
    LeaveCriticalSection(&m.handle);
    return 0;
  }

  int sc_gl_destroy_mutex(ScreenMutex& m) {
    DeleteCriticalSection(&m.handle);
    return 0;
  }

#elif defined(__linux) or defined(__APPLE__)

  int sc_gl_create_mutex(ScreenMutex& m) {

    if (0 != pthread_mutex_init(&m.handle, NULL)) {
      printf("Error: failed to create the mutex to sync the pixel data in ScreenCaptureGL.\n");
      return -1;
    }

    return 0;
  }

  int sc_gl_lock_mutex(ScreenMutex& m) {
    if (0 != pthread_mutex_lock(&m.handle)) {
      printf("Error: failed to lock the mutex to sync the pixel data in ScreenCaptureGL.\n");
      return -1;
    }

    return 0;
  }

  int sc_gl_unlock_mutex(ScreenMutex& m) {
    if (0 != pthread_mutex_unlock(&m.handle)) {
      printf("Error: failed to unlock the mutex to sync the pixel data in ScreenCaptureGL.\n");
      return -1;
    }

    return 0;
  }

  int sc_gl_destroy_mutex(ScreenMutex& m) {
    
    if (0 != pthread_mutex_destroy(&m.handle)) {
      printf("Error: failed to destroy the mutex to sync the pixel data in ScreenCaptureGL.\n");
      return -2;
    }

    return 0;
  }

  #endif

  
  /* --------------------------------------------------------------------------- */
  
  static void sc_gl_frame_callback(PixelBuffer& buffer) {

    ScreenCaptureGL* gl = static_cast<ScreenCaptureGL*>(buffer.user);
    if (NULL == gl) {
      printf("Error: failed to cast the user pointer to a ScreenCapture*\n");
      return;
    }

    if (SC_BGRA == buffer.pixel_format) {
      
      if (0 == buffer.nbytes[0]) {
        printf("Error: the number of bytes hasn't been set in the ScreenCapture.\n");
        return;
      }
      
      if (NULL == buffer.plane[0]) {
        printf("Error: the plane data pointer is NULL.\n");
        return;
      }
      
      sc_gl_lock_mutex(gl->mutex);
      {
        memcpy(gl->pixels, buffer.plane[0], buffer.nbytes[0]);
        gl->has_new_frame = true;
      }
      sc_gl_unlock_mutex(gl->mutex);
    }

  }
  
  /* --------------------------------------------------------------------------- */
} /* namespace sc */

#endif
