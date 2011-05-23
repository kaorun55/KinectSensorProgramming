#include <iostream>
#include <stdexcept>

#include <OpenGL/OpenGL.h>
#include <GLUT/GLUT.h>

#include <XnCppWrapper.h>

// 設定ファイルのパス(環境に合わせて変更してください)
#if (XN_PLATFORM == XN_PLATFORM_WIN32)
const char* CONFIG_XML_PATH = "../../../Data/SamplesConfig.xml";
#elif (XN_PLATFORM == XN_PLATFORM_MACOSX)
const char* CONFIG_XML_PATH = "../../../../../Data/SamplesConfig.xml";
#elif (XN_PLATFORM == XN_PLATFORM_LINUX_X86)
const char* CONFIG_XML_PATH = "../../../Data/SamplesConfig.xml";
#else
const char* CONFIG_XML_PATH = "Data/SamplesConfig.xml";
#endif

xn::Context g_context;
xn::ImageGenerator g_image;
XnMapOutputMode g_outputMode;

// アイドル時の処理
void idle()
{
  // display()を呼び出す
  ::glutPostRedisplay();
}

// 描画処理
void display()
{
  // カメライメージの更新を待ち、画像データを取得する ... (4)
  g_context.WaitOneUpdateAll( g_image );
  xn::ImageMetaData imageMD;
  g_image.GetMetaData( imageMD );

  // カメラ画像の表示 ... (5)
  ::glClear( GL_COLOR_BUFFER_BIT );

  ::glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
  ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  ::glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, imageMD.XRes(), imageMD.YRes(),
    0, GL_RGB, GL_UNSIGNED_BYTE, imageMD.RGB24Data() );

  ::glBegin(GL_QUADS);
  ::glTexCoord2f(0, 0);    ::glVertex2f(-1,  1);    // upper left
  ::glTexCoord2f(0, 1);    ::glVertex2f(-1, -1);    // bottom left
  ::glTexCoord2f(1, 1);    ::glVertex2f( 1, -1);    // bottom right
  ::glTexCoord2f(1, 0);    ::glVertex2f( 1,  1);    // upper right
  ::glEnd();

  ::glutSwapBuffers();
}

// キーボード処理
void keyboard(unsigned char key, int x, int y)
{
  if ( key == 'q' ) {
    exit(0);
  }
}

// OpenNI関連の初期化
void xnInit()
{
  // Kinectの初期化 ... (1)
  XnStatus rc = g_context.InitFromXmlFile(CONFIG_XML_PATH);
  if (rc != XN_STATUS_OK) {
    throw std::runtime_error( xnGetStatusString( rc ) );
  }

  // イメージジェネレータの作成 ... (2)
  rc = g_context.FindExistingNode(XN_NODE_TYPE_IMAGE, g_image);
  if (rc != XN_STATUS_OK) {
    throw std::runtime_error(xnGetStatusString(rc));
  }

  // カメラ画像のサイズを取得する ... (3)
  g_image.GetMapOutputMode(g_outputMode);
}

int main (int argc, char * argv[])
{
  try {
    ::xnInit();

    ::glutInit(&argc, argv);
    // カメラサイズのイメージを作成 ... (3)
    ::glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    ::glutInitWindowSize(g_outputMode.nXRes, g_outputMode.nYRes);

    ::glutCreateWindow("Camera Image");
    ::glutDisplayFunc(&::display);
    ::glutIdleFunc(&::idle);
    ::glutKeyboardFunc(&::keyboard);

    ::glEnable(GL_TEXTURE_2D);

    ::glutMainLoop();
  }
  catch (std::exception& ex) {
    std::cout << ex.what() << std::endl;
  }

  return 0;
}
