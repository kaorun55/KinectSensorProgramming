#include <iostream>
#include <stdexcept>

#include <map>
#include <list>

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

typedef std::map<int, std::list<XnPoint3D> > hand_point;

// ジェスチャーの検出中
void XN_CALLBACK_TYPE GestureProgress(xn::GestureGenerator& gesture,
  const XnChar* strGesture,
  const XnPoint3D* pPosition,
  XnFloat fProgress,
  void* pCookie)
{
  std::cout << "GestureProgress:" << strGesture << ", " << fProgress << std::endl;
}

// ジェスチャーの検出
void XN_CALLBACK_TYPE GestureRecognized(xn::GestureGenerator& gesture,
  const XnChar* strGesture,
  const XnPoint3D* pIDPosition,
  const XnPoint3D* pEndPosition,
  void* pCookie)
{
  std::cout << "GestureRecognized:" << strGesture << std::endl;

  xn::HandsGenerator* hands = (xn::HandsGenerator*)pCookie;
  hands->StartTracking(*pEndPosition);
}

// 手の検出開始
void XN_CALLBACK_TYPE HandCreate(xn::HandsGenerator& hands,
  XnUserID nId,
  const XnPoint3D* pPosition,
  XnFloat fTime,
  void* pCookie)
{
  std::cout << "HandCreate:" << nId << std::endl;
}

// 手の検出データの更新
void XN_CALLBACK_TYPE HandUpdate(xn::HandsGenerator& hands,
  XnUserID nId,
  const XnPoint3D* pPosition,
  XnFloat fTime,
  void* pCookie)
{
  //    std::cout << "HandUpdate:" << nId << std::endl;
  // 現在の座標をキューに追加
  hand_point& handPoint = *(hand_point*)pCookie;
  handPoint[nId].push_back(*pPosition);
}

// 手の検出終了
void XN_CALLBACK_TYPE HandDestroy(xn::HandsGenerator& hands,
  XnUserID nId,
  XnFloat fTime,
  void* pCookie)
{
  std::cout << "HandDestroy:" << nId << std::endl;

  // キューの初期化
  hand_point& handPoint = *(hand_point*)pCookie;
  handPoint[nId] = hand_point::mapped_type();

  // トラッキングの停止
  hands.StopTracking(nId);
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

    // デプスジェネレータの作成
    xn::DepthGenerator depth;
    rc = context.FindExistingNode(XN_NODE_TYPE_DEPTH, depth);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // ジェスチャージェネレータの作成
    xn::GestureGenerator gesture;
    rc = context.FindExistingNode(XN_NODE_TYPE_GESTURE, gesture);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // 検出するジェスチャーを追加する
    gesture.AddGesture("Click", 0);

    // ハンドジェネレータの作成
    xn::HandsGenerator hands;
    rc = context.FindExistingNode(XN_NODE_TYPE_HANDS, hands);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // ジェスチャー用のコールバックを登録する
    XnCallbackHandle gestureCallback, handsCallback, gestureChangeCallback;
    rc = gesture.RegisterGestureCallbacks(GestureRecognized, GestureProgress,
      &hands, gestureCallback);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // ハンドトラッキング用のコールバックを登録する
    hand_point handPoint;
    rc =hands.RegisterHandCallbacks(HandCreate, HandUpdate, HandDestroy,
      &handPoint, handsCallback);
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

      // 座標を描画
      for (hand_point::iterator user = handPoint.begin();
        user != handPoint.end(); ++user ){
          for (hand_point::mapped_type::iterator it = user->second.begin();
            it != user->second.end();  ) {
              XnPoint3D pt;
              depth.ConvertRealWorldToProjective(1, &*it, &pt);
              CvPoint pt1 = cvPoint(pt.X, pt.Y);
              CvPoint pt2 = cvPoint(pt.X, pt.Y);
              if (++it != user->second.end()) {
                depth.ConvertRealWorldToProjective(1, &*it, &pt);
                pt2 = cvPoint(pt.X, pt.Y);
              }

              ::cvLine(camera, pt1, pt2, CV_RGB(255, 0, 0), 5);
          }

          // 描画の最大点数は30とする
          const int MAX_POINT = 30;
          if (user->second.size() >= MAX_POINT) {
            user->second.erase(user->second.begin());
          }
      }

      //  Kinectからの入力がRGBであるため、BGRに変換して表示する
      ::cvCvtColor(camera, camera, CV_RGB2BGR);
      ::cvShowImage("KinectImage", camera);

      // 終了する
      char key = cvWaitKey(10);
      if (key == 'q') {
        break;
      }
    }
  }
  catch (std::exception& ex) {
    std::cout << ex.what() << std::endl;
  }

  ::cvReleaseImage(&camera);

  return 0;
}
