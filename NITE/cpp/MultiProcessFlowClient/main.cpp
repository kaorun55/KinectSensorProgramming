#include <iostream>
#include <stdexcept>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <XnCppWrapper.h>
#include <XnVMultiProcessFlowClient.h>
#include <XnVPushDetector.h>

// セッションの開始を通知されるコールバック
void XN_CALLBACK_TYPE SessionStart(const XnPoint3D& pFocus, void* UserCxt)
{
  std::cout << "SessionStart" << std::endl;
}

// セッションの終了を通知されるコールバック
void XN_CALLBACK_TYPE SessionEnd(void* UserCxt)
{
  std::cout << "SessionEnd" << std::endl;
}

// プッシュイベントを通知されるコールバック
void XN_CALLBACK_TYPE Push(XnFloat fVelocity, XnFloat fAngle, void *UserCxt)
{
  std::cout << "Push:" << fVelocity << "," << fAngle << std::endl;
}

// プッシュ後、停止したことを通知されるコールバック
void XN_CALLBACK_TYPE Stabilized(XnFloat fVelocity, void *UserCxt)
{
  std::cout << "Stabilized:" << fVelocity << std::endl;
}


int main (int argc, char * const argv[]) {
  IplImage* camera = 0;

  try {
    /* OpenNIのコンテキストを初期化する */
    xn::Context context;
    XnStatus rc = context.Init();
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // イメージジェネレータの作成
    xn::ImageGenerator image;
    rc = image.Create(context);
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


    /* NITEのセッションマネージャーを初期化する */
    XnVMultiProcessFlowClient sessionManager("shared_memory");
    rc = sessionManager.Initialize();
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // セッションの開始終了を通知するコールバックを登録する
    XnVHandle sessionCallnack = sessionManager.RegisterSession(
      0, &SessionStart, &SessionEnd );

    /* 検出器を作成する */
    XnVPushDetector pushDetector;
    XnCallbackHandle pushCallback = pushDetector.RegisterPush(0, Push);
    XnCallbackHandle stabilizedCallback =
      pushDetector.RegisterStabilized(0, Stabilized);

    /* セッションマネージャーに検出器を登録する */
    sessionManager.AddListener(&pushDetector);

    std::cout << "Client Initialize Success" << std::endl;

    /* メインループ */
    while (1) {
      /* ステータスの読み込み */
      sessionManager.ReadState();

      xn::ImageMetaData imageMD;
      image.GetMetaData(imageMD);

      // カメラ画像の表示
      memcpy(camera->imageData, imageMD.RGB24Data(), camera->imageSize);
      ::cvCvtColor(camera, camera, CV_BGR2RGB);
      ::cvShowImage("MultiProcessFlowClient", camera);

      /* キーの取得 */
      char key = cvWaitKey(10);
      /* 終了する */
      if (key == 'q') {
        break;
      }
    }

    // 登録したコールバックを削除
    pushDetector.UnregisterPush(pushCallback);
    pushDetector.UnregisterStabilized(stabilizedCallback);
    sessionManager.UnregisterSession(sessionCallnack);
  }
  catch (std::exception& ex) {
    std::cout << ex.what() << std::endl;
  }

  ::cvReleaseImage(&camera);

  return 0;
}
