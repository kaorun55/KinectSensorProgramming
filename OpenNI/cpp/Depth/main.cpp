#include <iostream>
#include <sstream>
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

int main (int argc, char * argv[])
{
    IplImage* camera = 0;

    try {
        // コンテキストの作成
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

        // カメラサイズのイメージを作成(8bitのRGB)
        XnMapOutputMode outputMode;
        image.GetMapOutputMode(outputMode);
        camera = ::cvCreateImage(cvSize(outputMode.nXRes, outputMode.nYRes),
                                 IPL_DEPTH_8U, 3);
        if (!camera) {
            throw std::runtime_error("error : cvCreateImage");
        }

        // 描画用フォントの作成
        CvFont font;
        ::cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1, 1, 10);

        // メインループ
        while (1) {
            // すべての更新を待ち、画像およびデプスデータを取得する
            context.WaitAndUpdateAll();

            xn::ImageMetaData imageMD;
            image.GetMetaData(imageMD);

            xn::DepthMetaData depthMD;
            depth.GetMetaData(depthMD);

            // カメラ画像の表示
            //  OpenCVへの出力がBGRであるため、RGBに変換して表示する
            memcpy(camera->imageData, imageMD.RGB24Data(), camera->imageSize);
            ::cvCvtColor(camera, camera, CV_RGB2BGR);

            // 中心にある物体までの距離を測って表示する
            CvPoint point = cvPoint(imageMD.XRes() / 2, imageMD.YRes() / 2);
            ::cvCircle(camera, point, 10, cvScalar(0, 0, 0), -1);

            std::stringstream ss;
            ss << depthMD( point.x, point.y ) << "mm";
            ::cvPutText(camera, ss.str().c_str(), point,
                            &font, cvScalar(0, 0, 255) );

            ::cvShowImage("KinectImage", camera);

            // 'q'が押されたら終了する
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
