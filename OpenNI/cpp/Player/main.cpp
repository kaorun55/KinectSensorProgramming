#include <iostream>
#include <stdexcept>
#include <sstream>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <XnCppWrapper.h>

// 設定ファイルのパス(環境に合わせて変更してください)
#if (XN_PLATFORM == XN_PLATFORM_WIN32)
const char* RECORDE_PATH = "../../../Data/record.oni";
#elif (XN_PLATFORM == XN_PLATFORM_MACOSX)
const char* RECORDE_PATH = "../../../../../Data/record.oni";
#elif (XN_PLATFORM == XN_PLATFORM_LINUX_X86)
const char* RECORDE_PATH = "../../../Data/record.oni";
#else
const char* RECORDE_PATH = "Data/record.oni";
#endif

// ユーザーの色づけ
const XnFloat Colors[][3] =
{
  {1,1,1},    // ユーザーなし
  {1,1,0},  {1,0,0},  {1,.5,0},
  {0,1,1},  {0,0,1},  {0,1,0},
  {.5,1,0}, {0,.5,1}, {.5,0,1},
  {1,1,.5},
};

// 再生の終了を通知
void XN_CALLBACK_TYPE OnEndOfFileReached(xn::ProductionNode& node, void* pCookie)
{
  std::cout << "OnEndOfFileReached" << std::endl;
}

// ユーザー検出
void XN_CALLBACK_TYPE UserDetected(xn::UserGenerator& generator,
                                   XnUserID nId, void* pCookie)
{
  std::cout << "ユーザー検出:" << nId << " " << generator.GetNumberOfUsers() << "人目" << std::endl;
}

// ユーザー消失
void XN_CALLBACK_TYPE UserLost(xn::UserGenerator& generator,
                               XnUserID nId, void* pCookie)
{
  std::cout << "ユーザー消失:" << nId << std::endl;
}

// デプスのヒストグラムを作成
typedef std::vector<float> depth_hist;
depth_hist getDepthHistgram(const xn::DepthGenerator& depth,
                            const xn::DepthMetaData& depthMD)
{
  // デプスの傾向を計算する(アルゴリズムはNiSimpleViewer.cppを利用)
  const int MAX_DEPTH = depth.GetDeviceMaxDepth();
  depth_hist depthHist(MAX_DEPTH);

  unsigned int points = 0;
  const XnDepthPixel* pDepth = depthMD.Data();
  for (XnUInt y = 0; y < depthMD.YRes(); ++y) {
    for (XnUInt x = 0; x < depthMD.XRes(); ++x, ++pDepth) {
      if (*pDepth != 0) {
        depthHist[*pDepth]++;
        points++;
      }
    }
  }

  for (int i = 1; i < MAX_DEPTH; ++i) {
    depthHist[i] += depthHist[i-1];
  }

  if ( points != 0) {
    for (int i = 1; i < MAX_DEPTH; ++i) {
      depthHist[i] =
        (unsigned int)(256 * (1.0f - (depthHist[i] / points)));
    }
  }

  return depthHist;
}

int main (int argc, char * argv[])
{
  IplImage* camera = 0;

  try {
    // Contextの初期化
    xn::Context context;
    XnStatus rc = context.Init();
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // 記録されたファイルを開く
    rc = context.OpenFileRecording(RECORDE_PATH);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // プレーヤーの作成
    xn::Player player;
    rc = context.FindExistingNode(XN_NODE_TYPE_PLAYER, player);
    if (rc != XN_STATUS_OK) {
      throw std::runtime_error(xnGetStatusString(rc));
    }

    // サポートされるフォーマットの取得
    std::cout << "SupportedFormat:" <<
      player.GetSupportedFormat() << std::endl;

    // 再生速度の取得
    std::cout << "PlaybackSpeed:" << player.GetPlaybackSpeed() << std::endl;

    // 記録されているノードを列挙し、ジェネレータを作成する
    xn::ImageGenerator image;
    xn::DepthGenerator depth;
    xn::UserGenerator user;

    xn::NodeInfoList nodeList;
    player.EnumerateNodes(nodeList);
    for ( xn::NodeInfoList::Iterator it = nodeList.Begin();
      it != nodeList.End(); ++it ) {
        std::cout <<
          ::xnProductionNodeTypeToString( (*it).GetDescription().Type ) <<
          ", " <<
          (*it).GetInstanceName() << ", " <<
          (*it).GetDescription().strName << ", " <<
          (*it).GetDescription().strVendor << ", " <<
          std::endl;

        if ((*it).GetDescription().Type == XN_NODE_TYPE_IMAGE) {
          rc = context.FindExistingNode(XN_NODE_TYPE_IMAGE, image);
          if (rc != XN_STATUS_OK) {
            throw std::runtime_error(xnGetStatusString(rc));
          }
        }
        else if ((*it).GetDescription().Type == XN_NODE_TYPE_DEPTH) {
          rc = context.FindExistingNode(XN_NODE_TYPE_DEPTH, depth);
          if (rc != XN_STATUS_OK) {
            throw std::runtime_error(xnGetStatusString(rc));
          }

          // ユーザーの作成
          rc = user.Create(context);
          if ( rc != XN_STATUS_OK ) {
            throw std::runtime_error( xnGetStatusString( rc ) );
          }
          
          // ユーザー検出機能をサポートしているか確認
          if (!user.IsCapabilitySupported(XN_CAPABILITY_SKELETON)) {
            throw std::runtime_error("ユーザー検出をサポートしてません");
          }
          
          // ユーザー認識のコールバックを登録
          XnCallbackHandle userCallbacks;
          user.RegisterUserCallbacks(&::UserDetected, &::UserLost, 0, userCallbacks);
          
          // ジェスチャー検出の開始
          context.StartGeneratingAll();
        }
    }

    // 有効なジェネレータを取得する
    xn::MapGenerator* generator = 0;
    if (image.IsValid()) {
      generator = &image;
    }
    else if (depth.IsValid()) {
      generator = &depth;
    }
    else {
      throw std::runtime_error("有効なノードが生成されていません");
    }

    // 記録終了時の通知を登録する
    XnCallbackHandle hCallback;
    player.RegisterToEndOfFileReached(&::OnEndOfFileReached,
      0, hCallback);

    // カメラサイズのイメージを作成(8bitのRGB)
    XnMapOutputMode outputMode;
    generator->GetMapOutputMode(outputMode);
    camera = ::cvCreateImage(cvSize(outputMode.nXRes, outputMode.nYRes),
      IPL_DEPTH_8U, 3);
    if (!camera) {
      throw std::runtime_error("error : cvCreateImage");
    }

    // 描画用フォントの作成
    CvFont font;
    ::cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1, 1, 10);

    // 表示状態
    bool isShowImage = true;
    bool isShowDepth = true;
    bool isShowUser = true;

    // メインループ
    while (1) {
      // データの更新
      context.WaitAndUpdateAll();

      // 画像バッファを白にする
      memset(camera->imageData, 255, camera->imageSize);

      // イメージが有効であれば、画像を表示する
      if (image.IsValid() && isShowImage) {
        xn::ImageMetaData imageMD;
        image.GetMetaData(imageMD);

        memcpy(camera->imageData, imageMD.RGB24Data(), camera->imageSize);
        ::cvCvtColor(camera, camera, CV_RGB2BGR);
      }

      // デプスが有効であれば、ヒストグラムとユーザーを表示する
      if (depth.IsValid() && isShowDepth) {
        xn::DepthMetaData depthMD;
        depth.GetMetaData(depthMD);
        depth_hist depthHist = getDepthHistgram(depth, depthMD);

        // ユーザーデータの取得
        xn::SceneMetaData sceneMD;
        user.GetUserPixels(0, sceneMD);
        
        // デプスマップの表示
        char* image = camera->imageData;
        for (XnUInt y = 0; y < depthMD.YRes(); ++y) {
          for (XnUInt x = 0; x < depthMD.XRes(); ++x, image += 3) {
            const XnDepthPixel& depth = depthMD(x, y);
            if (depth != 0) {
              image[0] = 0;
              image[1] = depthHist[depthMD(x, y)];
              image[2] = depthHist[depthMD(x, y)];
            }

            // ユーザー表示
            XnLabel label = sceneMD(x, y);
            if (!isShowUser) {
              label = 0;
            }
            
            // 出力先に描画
            image[0] *= Colors[label][0];
            image[1] *= Colors[label][1];
            image[2] *= Colors[label][2];
          }
        }
      }

      // 現在のフレーム数を取得
      XnUInt32 frame = 0;
      player.TellFrame(generator->GetName(), frame);

      // 現在のタイムスタンプを取得
      XnUInt64 timestamp = 0;
      player.TellTimestamp(timestamp);

      // データの表示
      std::stringstream ss;
      ss << frame << "frame, " << timestamp << "micro sec";
      ::cvPutText(camera, ss.str().c_str(), cvPoint(0, 100),
        &font, cvScalar(0, 0, 255) );

      // 画像の表示
      ::cvShowImage("KinectImage", camera);

      // 'q'が押されたら終了する
      char key = cvWaitKey(10);
      if (key == 'q') {
        break;
      }
      // リピートをやめる
      else if (key == 'r') {
        player.SetRepeat(FALSE);
      }
      // 再生速度を上げる
      else if (key == '+') {
        player.SetPlaybackSpeed(player.GetPlaybackSpeed() + 0.1);
        std::cout << "PlaybackSpeed:" << player.GetPlaybackSpeed() <<
          std::endl;
      }
      // 再生速度を下げる
      else if (key == '-') {
        player.SetPlaybackSpeed(player.GetPlaybackSpeed() - 0.1);
        std::cout << "PlaybackSpeed:" << player.GetPlaybackSpeed() <<
          std::endl;
      }
      // タイムスタンプで最初に戻る
      else if (key == 't') {
        player.SeekToTimeStamp(0, XN_PLAYER_SEEK_SET);
      }
      // フレームで最初に戻る
      else if (key == 'f') {
        player.SeekToFrame(generator->GetName(), 0, XN_PLAYER_SEEK_SET);
      }
      // 表示する/しないの切り替え
      else if (key == 'i') {
        isShowImage = !isShowImage;
      }
      // デプスの表示/非表示を切り替え
      else if (key == 'd') {
        isShowDepth = !isShowDepth;
      }
      // ユーザーの表示/非表示を切り替え
      else if (key == 'u') {
        isShowUser = !isShowUser;
      }
    }
  }
  catch (std::exception& ex) {
    std::cout << ex.what() << std::endl;
  }

  ::cvReleaseImage(&camera);

  return 0;
}
