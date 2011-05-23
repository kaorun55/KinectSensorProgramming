// Windows の場合はReleaseコンパイルにすると
// 現実的な速度で動作します
#include <iostream>
#include <stdexcept>
#include <vector>

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

// ユーザーの色づけ
const XnFloat Colors[][3] =
{
	{1,1,1},    // ユーザーなし
	{0,1,1},  {0,0,1},  {0,1,0},
	{1,1,0},  {1,0,0},  {1,.5,0},
	{.5,1,0}, {0,.5,1}, {.5,0,1},
	{1,1,.5},
};

// ユーザー検出
void XN_CALLBACK_TYPE UserDetected(xn::UserGenerator& generator,
                                   XnUserID nId, void* pCookie)
{
    std::cout << "ユーザー検出:" << nId << " " <<
    generator.GetNumberOfUsers() << "人目" << std::endl;
    
    XnChar* pose = (XnChar*)pCookie;
    if (pose[0] != '¥0') {
        generator.GetPoseDetectionCap().StartPoseDetection(pose, nId);
    }
    else {
        generator.GetSkeletonCap().RequestCalibration(nId, TRUE);
    }
}

// ユーザー消失
void XN_CALLBACK_TYPE UserLost(xn::UserGenerator& generator,
                               XnUserID nId, void* pCookie)
{
    std::cout << "ユーザー消失:" << nId << std::endl;
}

// ポーズ検出
void XN_CALLBACK_TYPE PoseDetected(xn::PoseDetectionCapability& capability,
                                   const XnChar* strPose, XnUserID nId, void* pCookie)
{
    std::cout << "ポーズ検出:" << strPose << " ユーザー:" << nId << std::endl;
    
    xn::UserGenerator* user = (xn::UserGenerator*)pCookie;
    user->GetPoseDetectionCap().StopPoseDetection(nId);
    user->GetSkeletonCap().RequestCalibration(nId, TRUE);
}

// ポーズ消失
void XN_CALLBACK_TYPE PoseLost(xn::PoseDetectionCapability& capability,
                               const XnChar* strPose, XnUserID nId, void* pCookie)
{
    std::cout << "ポーズ消失:" << strPose << " ユーザー:" << nId << std::endl;
}

// キャリブレーションの開始
void XN_CALLBACK_TYPE CalibrationStart(xn::SkeletonCapability& capability,
                                       XnUserID nId, void* pCookie)
{
    std::cout << "キャリブレーション開始。ユーザー:" << nId << std::endl;
}

// キャリブレーションの終了
void XN_CALLBACK_TYPE CalibrationEnd(xn::SkeletonCapability& capability,
                                     XnUserID nId, XnBool bSuccess, void* pCookie)
{
    xn::UserGenerator* user = (xn::UserGenerator*)pCookie;
    
    // キャリブレーション成功
    if (bSuccess) {
        std::cout << "キャリブレーション成功。ユーザー:" << nId << std::endl;
        user->GetSkeletonCap().StartTracking(nId);
    }
    // キャリブレーション失敗
    else {
        std::cout << "キャリブレーション失敗。ユーザー:" << nId << std::endl;
    }
}

class SkeltonDrawer
{
public:
    SkeltonDrawer( IplImage* camera, xn::SkeletonCapability& skelton,
                  xn::DepthGenerator& depth, XnUserID player )
    :camera_(camera), skelton_(skelton), depth_(depth), player_(player)
    {
    }
    
    // スケルトンを描画する
    void draw()
    {
	    if (!skelton_.IsTracking(player_)) {
            throw std::runtime_error("トラッキングされていません");
	    }
        
        drawLine(XN_SKEL_HEAD, XN_SKEL_NECK);
        
        drawLine(XN_SKEL_NECK, XN_SKEL_LEFT_SHOULDER);
        drawLine(XN_SKEL_LEFT_SHOULDER, XN_SKEL_LEFT_ELBOW);
        drawLine(XN_SKEL_LEFT_ELBOW, XN_SKEL_LEFT_HAND);
        
        drawLine(XN_SKEL_NECK, XN_SKEL_RIGHT_SHOULDER);
        drawLine(XN_SKEL_RIGHT_SHOULDER, XN_SKEL_RIGHT_ELBOW);
        drawLine(XN_SKEL_RIGHT_ELBOW, XN_SKEL_RIGHT_HAND);
        
        drawLine(XN_SKEL_LEFT_SHOULDER, XN_SKEL_TORSO);
        drawLine(XN_SKEL_RIGHT_SHOULDER, XN_SKEL_TORSO);
        
        drawLine(XN_SKEL_TORSO, XN_SKEL_LEFT_HIP);
        drawLine(XN_SKEL_LEFT_HIP, XN_SKEL_LEFT_KNEE);
        drawLine(XN_SKEL_LEFT_KNEE, XN_SKEL_LEFT_FOOT);
        
        drawLine(XN_SKEL_TORSO, XN_SKEL_RIGHT_HIP);
        drawLine(XN_SKEL_RIGHT_HIP, XN_SKEL_RIGHT_KNEE);
        drawLine(XN_SKEL_RIGHT_KNEE, XN_SKEL_RIGHT_FOOT);
        
        drawLine(XN_SKEL_LEFT_HIP, XN_SKEL_RIGHT_HIP);
    }
    
private:
    // スケルトンの線を描画する
    void drawLine(XnSkeletonJoint eJoint1, XnSkeletonJoint eJoint2)
    {
        // 各箇所の座標を取得する
	    XnSkeletonJointPosition joint1, joint2;
	    skelton_.GetSkeletonJointPosition(player_, eJoint1, joint1);
	    skelton_.GetSkeletonJointPosition(player_, eJoint2, joint2);
	    if (joint1.fConfidence < 0.5 || joint2.fConfidence < 0.5) {
		    return;
	    }
        
        // 座標を変換する
        XnPoint3D pt[2] = { joint1.position, joint2.position };
	    depth_.ConvertRealWorldToProjective(2, pt, pt);
        cvLine(camera_,cvPoint(pt[0].X, pt[0].Y), cvPoint(pt[1].X, pt[1].Y),
               CV_RGB(0, 255, 255),2,CV_AA ,0);
    }
    
private:
    
    IplImage* camera_;
    xn::SkeletonCapability& skelton_;
    xn::DepthGenerator& depth_;
    XnUserID player_;
};

// RGBピクセルの初期化
inline XnRGB24Pixel xnRGB24Pixel( int r, int g, int b )
{
    XnRGB24Pixel pixel = { r, g, b };
    return pixel;
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
        
        // 鏡モード(反転)にする
        context.SetGlobalMirror(TRUE);
        
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
        
        // デプスの座標をイメージに合わせる
        depth.GetAlternativeViewPointCap().SetViewPoint(image);
        
        // ユーザーの作成
        xn::UserGenerator user;
        rc = context.FindExistingNode( XN_NODE_TYPE_USER, user );
        if ( rc != XN_STATUS_OK ) {
            rc = user.Create(context);
            if ( rc != XN_STATUS_OK ) {
                throw std::runtime_error( xnGetStatusString( rc ) );
            }
        }
        
        // ユーザー検出機能をサポートしているか確認
        if (!user.IsCapabilitySupported(XN_CAPABILITY_SKELETON)) {
            throw std::runtime_error("ユーザー検出をサポートしてません");
        }
        
        XnCallbackHandle userCallbacks, calibrationCallbacks, poseCallbacks;
        XnChar pose[20] = "";
        
        // キャリブレーションにポーズが必要
        xn::SkeletonCapability skelton = user.GetSkeletonCap();
        if (skelton.NeedPoseForCalibration()) {
            // ポーズ検出のサポートチェック
            if (!user.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION)) {
                throw std::runtime_error("ポーズ検出をサポートしてません");
            }
            
            // キャリブレーションポーズの取得
            skelton.GetCalibrationPose(pose);
            
            // ポーズ検出のコールバックを登録
            xn::PoseDetectionCapability pose = user.GetPoseDetectionCap();
            pose.RegisterToPoseCallbacks(&::PoseDetected, &::PoseLost,
                                         &user, poseCallbacks);
        }
        
        // ユーザー認識のコールバックを登録
        user.RegisterUserCallbacks(&::UserDetected, &::UserLost, pose,
                                   userCallbacks);
        
        // キャリブレーションのコールバックを登録
        skelton.RegisterCalibrationCallbacks(&::CalibrationStart, &::CalibrationEnd,
                                             &user, calibrationCallbacks);
        
        // ユーザートラッキングで、すべてをトラッキングする
        skelton.SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
        
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
        
        // 表示状態
        bool isShowImage = true;
        bool isShowUser = true;
        bool isShowSkelton = true;
        

        // 描画状態
        static enum State{
            IDLE = 0,
            CIRCLE,
        } state[15] = { IDLE };

        typedef std::vector<XnPoint3D> line;
        line points;
        CvScalar color = CV_RGB(0,0,0);
        
        typedef std::vector<CvScalar> LineColors;
        LineColors colors;
        colors.push_back(CV_RGB(255,255,255));
        colors.push_back(CV_RGB(255,255,255));
        colors.push_back(CV_RGB(0,0,0));
        colors.push_back(CV_RGB(255,0,0));
        colors.push_back(CV_RGB(0,255,0));
        colors.push_back(CV_RGB(0,0,255));
        colors.push_back(CV_RGB(255,255,0));
        colors.push_back(CV_RGB(0,255,255));
        colors.push_back(CV_RGB(255,0,255));
        
        // メインループ
        while (1) {
            // すべてのノードの更新を待つ
            context.WaitAndUpdateAll();
            
            // 画像データの取得
            xn::ImageMetaData imageMD;
            image.GetMetaData(imageMD);
            
            // ユーザーデータの取得
            xn::SceneMetaData sceneMD;
            user.GetUserPixels(0, sceneMD);
            
            // カメラ画像の表示
            char* dest = camera->imageData;
            const xn::RGB24Map& rgb = imageMD.RGB24Map();
            for (int y = 0; y < imageMD.YRes(); ++y) {
                for (int x = 0; x < imageMD.XRes(); ++x) {
                    // ユーザー表示
                    XnLabel label = sceneMD(x, y);
                    if (!isShowUser) {
                        label = 0;
                    }
                    
                    // カメラ画像の表示
                    XnRGB24Pixel pixel = rgb(x, y);
                    if (!isShowImage) {
                        pixel = xnRGB24Pixel( 255, 255, 255 );
                    }
                    
                    // 出力先に描画
                    dest[0] = pixel.nRed   * Colors[label][0];
                    dest[1] = pixel.nGreen * Colors[label][1];
                    dest[2] = pixel.nBlue  * Colors[label][2];
                    dest += 3;
                }
            }
            
            // スケルトンの描画
            if (isShowSkelton) {
                XnUserID aUsers[15];
                XnUInt16 nUsers = 15;
                user.GetUsers(aUsers, nUsers);
//                for (int i = 0; i < nUsers; ++i) {
                for (int i = 0; i < 1; ++i) {
                    if (skelton.IsTracking(aUsers[i])) {
                        SkeltonDrawer skeltonDrawer(camera, skelton,
                                                    depth, aUsers[i]);
                        skeltonDrawer.draw();

                        // 右手と右肩の距離を表示
                        XnSkeletonJointPosition shoulder, l_shoulder, r_hand, l_hand;
                        skelton.GetSkeletonJointPosition(aUsers[i], XN_SKEL_RIGHT_SHOULDER, shoulder);
                        skelton.GetSkeletonJointPosition(aUsers[i], XN_SKEL_RIGHT_HAND, r_hand);
                        skelton.GetSkeletonJointPosition(aUsers[i], XN_SKEL_LEFT_SHOULDER, l_shoulder);
                        skelton.GetSkeletonJointPosition(aUsers[i], XN_SKEL_LEFT_HAND, l_hand);

                        // 現実の座標を画面座標に変換する
                        XnPoint3D pt_r_hand, pt_l_hand;
                        depth.ConvertRealWorldToProjective(1, &r_hand.position, &pt_r_hand);
                        depth.ConvertRealWorldToProjective(1, &l_hand.position, &pt_l_hand);

                        // 右肩と右手の間隔が40cm以上(手を前に出してる感じ)
                        if ((shoulder.position.Z - r_hand.position.Z) >= 400) {
                            std::cout << " CIRCLE";
                            state[i] = CIRCLE;
                        }
                        // 右肩と右手の間隔が40cm以上(手を前に出してる感じ)
                        if ((l_shoulder.position.Z - l_hand.position.Z) >= 400) {
                            std::cout << " IDLE";
                            state[i] = IDLE;
                        }
                        
                        
                        // 左上に左手を持ってたら色を変える
                        int r = 50;
                        for (int c = 0; c < colors.size(); ++c) {
                            cvRectangle(camera, cvPoint(c * r, 0), cvPoint((c+1)*r, r), colors[c], CV_FILLED);
                        }
                        if (pt_l_hand.Y < r) {
                            int index = pt_l_hand.X / r;
                            if (index == 0) {
                                points.clear();
                            }
                            else if (index < colors.size()) {
                                std::cout << " change color";
                                color = colors[index];
                            }
                        }
                        
                        std::cout << std::endl;

                        if (state[i] == CIRCLE) {
                            points.push_back(pt_r_hand);
                            cvCircle(camera, cvPoint(pt_r_hand.X, pt_r_hand.Y), 10, CV_RGB(255, 255, 0), 5);
                        }

                        // 線を書く
                        for (line::iterator it = points.begin();it != points.end();){
                            CvPoint pt1 = cvPoint(it->X, it->Y);
                            CvPoint pt2 = cvPoint(it->X, it->Y);
                            if (++it != points.end()) {
                                pt2 = cvPoint(it->X, it->Y);
                            }
                            
                            ::cvLine(camera, pt1, pt2, color, 3);
                        }
                    }
                }
            }
            
            ::cvCvtColor(camera, camera, CV_BGR2RGB);
            ::cvShowImage("KinectImage", camera);
            
            // キーイベント
            char key = cvWaitKey(10);
            // 終了する
            if (key == 'q') {
                break;
            }
            // 反転する
            else if (key == 'm') {
                context.SetGlobalMirror(!context.GetGlobalMirror());
            }
            // 表示する/しないの切り替え
            else if (key == 'i') {
                isShowImage = !isShowImage;
            }
            else if (key == 'u') {
                isShowUser = !isShowUser;
            }
            else if (key == 's') {
                isShowSkelton = !isShowSkelton;
            }
        }
    }
    catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }
    
    ::cvReleaseImage(&camera);
    
    return 0;
}
