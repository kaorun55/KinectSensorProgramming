#include <iostream>
#include <stdexcept>

#include <opencv/cv.h>
#include <opencv/highgui.h>

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

// ジェスチャーの検出中
void XN_CALLBACK_TYPE GestureProgress(xn::GestureGenerator& gesture,
                              const XnChar* strGesture,
                              const XnPoint3D* pPosition,
                              XnFloat fProgress,
                              void* pCookie)
{
  std::cout << "GestureProgress:" << strGesture << ", " << fProgress << std::endl;
}

// ジェスチャーを検出した
void XN_CALLBACK_TYPE GestureRecognized(xn::GestureGenerator& gesture,
                              const XnChar* strGesture,
                              const XnPoint3D* pIDPosition,
                              const XnPoint3D* pEndPosition,
                              void* pCookie)
{
  std::cout << "GestureRecognized:" << strGesture << std::endl;
}

// 登録されるジェスチャーが変わった
void XN_CALLBACK_TYPE GestureChange(xn::ProductionNode& node, void* pCookie)
{
  std::cout << "GestureChange" << std::endl;
}

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

    // ジェスチャージェネレータの作成
    xn::GestureGenerator gesture;
    rc = context.FindExistingNode(XN_NODE_TYPE_GESTURE, gesture);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // サポートされているジェスチャーを取得する
    // 数はとれるが、ジェスチャー名が取れない
    const int GESTURE_COUNT = 32, NAME_LENGTH = 32;
    XnUInt16 supportGesutureCount = GESTURE_COUNT;
    XnChar supportGestureName[GESTURE_COUNT][NAME_LENGTH] = { 0 };
    rc = gesture.EnumerateAllGestures((XnChar**)supportGestureName, NAME_LENGTH,
                                                            supportGesutureCount);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    std::cout << "supportGesutureCount:" << supportGesutureCount << std::endl;
    for (XnUInt16 i = 0; i < supportGesutureCount; ++i) {
      if (supportGestureName[i][0] != '\0') {
        std::cout << supportGestureName[i] << std::endl;
      }
    }

    
    // 検出するジェスチャーを追加する
    std::vector<std::string> gestures;
    int gestureIndex = 0;
    gestures.push_back("Click");
    gestures.push_back("Wave");
    gestures.push_back("RaiseHand");
    gestures.push_back("MovingHand");

    gesture.AddGesture(gestures[gestureIndex].c_str(), 0);

    // ジェスチャーの機能確認
    for ( std::vector<std::string>::const_iterator it = gestures.begin();
      it != gestures.end(); ++it ) {
        std::cout << *it << ":" << 
          "Available:" << gesture.IsGestureAvailable(it->c_str()) << 
          " ProgressSupported:" << gesture.IsGestureProgressSupported(it->c_str()) <<
          std::endl;
    }

    // アクティブなジェスチャーの検出
    // 数はとれるが、ジェスチャー名が取れない
    XnChar gestureName[GESTURE_COUNT][NAME_LENGTH] = { "" };
    XnUInt16 gesutureCount = GESTURE_COUNT;
    rc = gesture.GetAllActiveGestures((XnChar**)gestureName, NAME_LENGTH,
                                                            gesutureCount);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    std::cout << "gesutureCount:" << gesutureCount << std::endl;
    for (XnUInt16 i = 0; i < gesutureCount; ++i) {
      if (gestureName[i][0] != '\0') {
        std::cout << gestureName[i] << std::endl;
      }
    }

    // ジェスチャー検出用のコールバックを登録する
    XnCallbackHandle gestureCallback, gestureChangeCallback;
    rc = gesture.RegisterGestureCallbacks(GestureRecognized,
                            GestureProgress, 0, gestureCallback);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // ジェスチャー切り替え検出用のコールバックを登録する
    rc = gesture.RegisterToGestureChange(GestureChange, 0,
      gestureChangeCallback);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // ジェスチャー検出の開始
    context.StartGeneratingAll();

    // カメラサイズのイメージを作成(8bitのRGB)
    XnMapOutputMode outputMode;
    image.GetMapOutputMode(outputMode);
    camera = ::cvCreateImage(cvSize(outputMode.nXRes, outputMode.nYRes),
      IPL_DEPTH_8U, 3);
    if (!camera) {
      throw std::runtime_error("error : cvCreateImage");
    }

    // メインループ
    while (1) {
      // カメライメージの更新を待ち、画像データを取得する
      context.WaitOneUpdateAll(image);

      xn::ImageMetaData imageMD;
      image.GetMetaData(imageMD);

      // カメラ画像の表示
      memcpy(camera->imageData, imageMD.RGB24Data(), camera->imageSize);
      ::cvCvtColor(camera, camera, CV_BGR2RGB);
      ::cvShowImage("KinectImage", camera);

      // 終了する
      char key = cvWaitKey(10);
      if (key == 'q') {
        break;
      }
      // ジェスチャーの種類を変更する
      else if (key == 'g') {
        // ジェスチャーを削除して、次のジェスチャーを登録する
        gesture.StopGenerating();
        gesture.RemoveGesture(gestures[gestureIndex].c_str());

        ++gestureIndex;
        if (gestureIndex >= gestures.size()) {
          gestureIndex = 0;
        }

        gesture.AddGesture(gestures[gestureIndex].c_str(), 0);
        gesture.StartGenerating();

        std::cout << "GestureChange:" << gestures[gestureIndex] << std::endl;
      }
    }

    // 登録したコールバックを削除する
    gesture.UnregisterFromGestureChange(gestureChangeCallback);
    gesture.UnregisterGestureCallbacks(gestureCallback);
  }
  catch (std::exception& ex) {
    std::cout << ex.what() << std::endl;
  }

  ::cvReleaseImage(&camera);

  return 0;
}
