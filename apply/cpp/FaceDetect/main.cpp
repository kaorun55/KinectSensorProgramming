#include <iostream>
#include <stdexcept>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <XnCppWrapper.h>

// 設定ファイルのパス(環境に合わせて変更してください)
#if (XN_PLATFORM == XN_PLATFORM_WIN32)
const char* CONFIG_XML_PATH = "../../../Data/SamplesConfig.xml";
const char* FACE_CASCADE_PATH = "C:/OpenCV2.1/data/haarcascades/haarcascade_frontalface_alt.xml";
#elif (XN_PLATFORM == XN_PLATFORM_MACOSX)
const char* CONFIG_XML_PATH = "../../../../../Data/SamplesConfig.xml";
const char* FACE_CASCADE_PATH = "/usr/local/share/opencv/haarcascades/haarcascade_frontalface_alt.xml";
#elif (XN_PLATFORM == XN_PLATFORM_LINUX_X86)
const char* CONFIG_XML_PATH = "../../../Data/SamplesConfig.xml";
const char* FACE_CASCADE_PATH = "/usr/local/share/opencv/haarcascades/haarcascade_frontalface_alt.xml";
#else
const char* CONFIG_XML_PATH = "Data/SamplesConfig.xml";
const char* FACE_CASCADE_PATH = "/usr/local/share/opencv/haarcascades/haarcascade_frontalface_alt.xml";
#endif

int main (int argc, char * argv[])
{
  IplImage* camera = 0;

  try {
    // コンテキストの初期化
    xn::Context context;
    XnStatus rc = context.InitFromXmlFile(CONFIG_XML_PATH);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // イメージジェネレータの作成
    xn::ImageGenerator image;
    rc = context.FindExistingNode(XN_NODE_TYPE_IMAGE, image);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // カメラサイズのイメージを作成(8bitのRGB)
    XnMapOutputMode outputMode;
    image.GetMapOutputMode(outputMode);
    camera = ::cvCreateImage(cvSize(outputMode.nXRes, outputMode.nYRes),
      IPL_DEPTH_8U, 3);
    if (!camera) {
      throw std::runtime_error("error : cvCreateImage");
    }


    // 検出器のロード
    CvHaarClassifierCascade* faceCascade = (CvHaarClassifierCascade*)
      ::cvLoad(FACE_CASCADE_PATH, 0, 0, 0 );
    if( !faceCascade ) {
      throw std::runtime_error("error : cvLoad");
    }

    // 顔検出用のストレージ
    CvMemStorage* storage = ::cvCreateMemStorage();
    bool isDetected = true;

    // メインループ
    while (1) {
      // カメライメージの更新を待ち、画像データを取得する
      context.WaitOneUpdateAll(image);

      xn::ImageMetaData imageMD;
      image.GetMetaData(imageMD);

      // カメラ画像の表示
      //  Kinectからの入力がBGRであるため、RGBに変換して表示する
      memcpy(camera->imageData, imageMD.RGB24Data(), camera->imageSize);
      ::cvCvtColor(camera, camera, CV_BGR2RGB);

      // 顔の検出と検出された顔領域の描画
      if (isDetected) {
        cvClearMemStorage(storage);
        CvSeq* faces = ::cvHaarDetectObjects(camera, faceCascade, storage);
        for ( int i = 0; i < faces->total; ++i ) {
          CvRect rect = *(CvRect*)::cvGetSeqElem( faces, i );
          ::cvRectangle(camera, ::cvPoint( rect.x, rect.y ),
            ::cvPoint( rect.x + rect.width, rect.y + rect.height ),
            CV_RGB( 255, 0, 0 ), 3 );
        }
      }

      ::cvShowImage("KinectImage", camera);


      // キーの取得
      char key = cvWaitKey(10);
      // 終了する
      if (key == 'q') {
        break;
      }
      else if (key == 'd') {
        isDetected = !isDetected;
      }
    }
  }
  catch (std::exception& ex) {
    std::cout << ex.what() << std::endl;
  }

  ::cvReleaseImage(&camera);

  return 0;
}
