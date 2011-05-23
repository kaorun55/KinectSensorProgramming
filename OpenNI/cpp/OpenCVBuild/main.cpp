#include <stdexcept>
#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>

int main(int argc, char* argv[])
{
  try {
    // 読み込み画像ファイル名
    // 適当なファイル名に置き換えてください
#ifdef _WIN32
    const char *fileName = "C:\\OpenCV2.1\\samples\\c\\lena.jpg";
#elif __APPLE__ & __MACH__
    const char *fileName = "/Users/kaorun55/Downloads/OpenCV-2.1.0/samples/c/lena.jpg";
#else
    const char *fileName = "/home/kaorun55/OpenCV-2.1.0/samples/c/lena.jpg";
#endif

    // 表示ウィンドウ名
    const char *windowName = "lena";

    // 画像の読み込み
    ::IplImage* img = ::cvLoadImage( fileName );
    if ( img == 0 ) {
      throw std::runtime_error( "can not open image file" );
    }

    // 画像の表示
    ::cvNamedWindow( windowName );
    ::cvShowImage( windowName, img );
    ::cvWaitKey();
    ::cvDestroyWindow( windowName );

    // 画像の解放
    ::cvReleaseImage( &img );
  }
  catch ( std::exception& ex ) {
    std::cout << ex.what() << std::endl;
  }

  return 0;
}

