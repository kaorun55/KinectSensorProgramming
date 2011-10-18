using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;
using OpenNI;

namespace Calibration
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

    private Context context;
    private ImageGenerator image;
    private DepthGenerator depth;
    private UserGenerator user;
    private SkeletonCapability skelton;

    private string pose = string.Empty;

    // 描画用
    private Pen pen = new Pen(Color.White, 5);

    private Brush brush = new SolidBrush(Color.Black);
    private Font font = new Font("Arial", 30);
    private PointF point = new PointF(0, 0);

    private string message = "";

    // ユーザーの色づけ
    float[,] colors = new float[,]
        {
	        {1,1,1},    // ユーザーなし
	        {0,1,1},    {0,0,1},    {0,1,0},
	        {1,1,0},    {1,0,0},    {1,0.5F,0},
	        {0.5F,1,0}, {0,0.5F,1}, {0.5F,0,1},
	        {1,1,0.5F},
        };

    // 初期化
    private void xnInitialize()
    {
      // コンテキストの初期化
      ScriptNode scriptNode;
      context = Context.CreateFromXmlFile( CONFIG_XML_PATH, out scriptNode );

      // 鏡モード(反転)にしない
      context.GlobalMirror = false;

      // イメージジェネレータの作成
      image = context.FindExistingNode(NodeType.Image) as ImageGenerator;
      if (image == null) {
        throw new Exception(context.GlobalErrorState);
      }

      // デプスジェネレータの作成
      depth = context.FindExistingNode(NodeType.Depth) as DepthGenerator;
      if (depth == null) {
        throw new Exception(context.GlobalErrorState);
      }

      // デプスの座標をイメージに合わせる
      depth.AlternativeViewpointCapability.SetViewpoint(image);

      // ユーザージェネレータの作成
      user = context.FindExistingNode(NodeType.User) as UserGenerator;
      if (depth == null) {
        throw new Exception(context.GlobalErrorState);
      }

      // ユーザー検出機能をサポートしているか確認
      if (!user.IsCapabilitySupported("User::Skeleton")) {
        throw new Exception("ユーザー検出をサポートしていません");
      }

      // ユーザー認識のコールバックを登録
      user.NewUser += new EventHandler<NewUserEventArgs>(user_NewUser);
      user.LostUser += new EventHandler<UserLostEventArgs>(user_LostUser);

      //キャリブレーションにポーズが必要か確認
      skelton = user.SkeletonCapability;
      if (skelton.DoesNeedPoseForCalibration) {
        // ポーズ検出のサポートチェック
        if (!user.IsCapabilitySupported("User::PoseDetection")) {
          throw new Exception("ユーザー検出をサポートしていません");
        }

        // キャリブレーションポーズの取得
        pose = skelton.CalibrationPose;

        // ポーズ検出のコールバックを登録
        PoseDetectionCapability poseDetect = user.PoseDetectionCapability;
        poseDetect.PoseDetected += new EventHandler<PoseDetectedEventArgs>(poseDetect_PoseDetected);
        poseDetect.OutOfPose += new EventHandler<OutOfPoseEventArgs>( poseDetect_OutOfPose );
      }

      // キャリブレーションのコールバックを登録
      skelton.CalibrationStart += new EventHandler<CalibrationStartEventArgs>(skelton_CalibrationStart);
      skelton.CalibrationComplete += new EventHandler<CalibrationProgressEventArgs>( skelton_CalibrationComplete );

      // すべてをトラッキングする
      skelton.SetSkeletonProfile(SkeletonProfile.All);

      // ジェスチャーの検出開始
      context.StartGeneratingAll();
    }

    // 描画
    private unsafe void xnDraw()
    {
      // カメライメージの更新を待ち、画像データを取得する
      context.WaitOneUpdateAll(image);
      ImageMetaData imageMD = image.GetMetaData();
      SceneMetaData sceneMD = user.GetUserPixels(0);

      // カメラ画像の作成
      lock (this) {
        // 書き込み用のビットマップデータを作成
        Rectangle rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        BitmapData data = bitmap.LockBits(rect, ImageLockMode.WriteOnly,
                        System.Drawing.Imaging.PixelFormat.Format24bppRgb);

        // 生データへのポインタを取得
        byte* dst = (byte*)data.Scan0.ToPointer();
        byte* src = (byte*)image.ImageMapPtr.ToPointer();
        ushort* label = (ushort*)sceneMD.LabelMapPtr.ToPointer();

        for (int i = 0; i < imageMD.DataSize; i += 3, src += 3, dst += 3, ++label) {
          dst[0] = (byte)(src[2] * colors[*label, 0]);
          dst[1] = (byte)(src[1] * colors[*label, 1]);
          dst[2] = (byte)(src[0] * colors[*label, 2]);
        }

        bitmap.UnlockBits(data);

        // スケルトンの描画
        foreach (int id in user.GetUsers()) {
          // トラッキング対象のユーザーでなければ次へ
          if (!user.SkeletonCapability.IsTracking(id)) {
            continue;
          }

          DrawLine(id, SkeletonJoint.Head, SkeletonJoint.Neck);

          DrawLine(id, SkeletonJoint.Neck, SkeletonJoint.LeftShoulder);
          DrawLine(id, SkeletonJoint.LeftShoulder, SkeletonJoint.LeftElbow);
          DrawLine(id, SkeletonJoint.LeftElbow, SkeletonJoint.LeftHand);

          DrawLine(id, SkeletonJoint.Neck, SkeletonJoint.RightShoulder);
          DrawLine(id, SkeletonJoint.RightShoulder, SkeletonJoint.RightElbow);
          DrawLine(id, SkeletonJoint.RightElbow, SkeletonJoint.RightHand);

          DrawLine(id, SkeletonJoint.LeftShoulder, SkeletonJoint.Torso);
          DrawLine(id, SkeletonJoint.RightShoulder, SkeletonJoint.Torso);

          DrawLine(id, SkeletonJoint.Torso, SkeletonJoint.LeftHip);
          DrawLine(id, SkeletonJoint.LeftHip, SkeletonJoint.LeftKnee);
          DrawLine(id, SkeletonJoint.LeftKnee, SkeletonJoint.LeftFoot);

          DrawLine(id, SkeletonJoint.Torso, SkeletonJoint.RightHip);
          DrawLine(id, SkeletonJoint.RightHip, SkeletonJoint.RightKnee);
          DrawLine(id, SkeletonJoint.RightKnee, SkeletonJoint.RightFoot);

          DrawLine(id, SkeletonJoint.LeftHip, SkeletonJoint.RightHip);
        }


        // 現在の状態を表示する
        Graphics g = Graphics.FromImage(bitmap);
        g.DrawString(message, font, brush, point);
      }
    }

    // キーイベント
    private void xnKeyDown(Keys key)
    {
    }



    // ユーザー検出
    void user_NewUser(object sender, NewUserEventArgs e)
    {
      message = "ユーザー検出:" + e.ID;

      // ポーズの検出が必要であれば、ポーズの検出を開始する
      if (!string.IsNullOrEmpty(pose))
      {
        user.PoseDetectionCapability.StartPoseDetection(pose, e.ID);
      }
      // ポーズの検出が不要であれば、キャリブレーションを開始する
      else
      {
        user.SkeletonCapability.RequestCalibration(e.ID, true);
      }
    }

    // ユーザー消失
    void user_LostUser(object sender, UserLostEventArgs e)
    {
      message = "ユーザー消失:" + e.ID;
    }

    // ポーズ検出
    void poseDetect_PoseDetected(object sender, PoseDetectedEventArgs e)
    {
      message = "ポーズ検出:" + e.Pose + " ユーザー:" + e.ID;

      // ポーズの検出を停止し、キャリブレーションを開始する
      user.PoseDetectionCapability.StopPoseDetection(e.ID);
      user.SkeletonCapability.RequestCalibration(e.ID, true);
    }

    // ポーズ検出終了
    void poseDetect_OutOfPose( object sender, OutOfPoseEventArgs e )
    {
      message = "ポーズ消失:" + e.Pose + " ユーザー:" + e.ID;
    }

    // キャリブレーション開始
    void skelton_CalibrationStart(object sender, CalibrationStartEventArgs e)
    {
      message = "キャリブレーション開始:" + e.ID;
    }

    // キャリブレーション終了
    void skelton_CalibrationComplete( object sender, CalibrationProgressEventArgs e )
    {
      // キャリブレーション成功
      if (e.Status == CalibrationStatus.OK) {
        message = "キャリブレーション成功:" + e.ID;
        user.SkeletonCapability.StartTracking(e.ID);
      }
      // キャリブレーション失敗
      else {
        message = "キャリブレーション失敗:" + e.ID;
      }
    }

    // 骨格の線を引く
    void DrawLine(int player, SkeletonJoint eJoint1, SkeletonJoint eJoint2)
    {
      // 各箇所の座標を取得する
      SkeletonJointPosition joint1 = skelton.GetSkeletonJointPosition(player, eJoint1);
      SkeletonJointPosition joint2 = skelton.GetSkeletonJointPosition(player, eJoint2);
      if (joint1.Confidence < 0.5 || joint2.Confidence < 0.5) {
        return;
      }

      // 現実の座標から画面の座標に変換する
      Point3D[] pt = new Point3D[] { joint1.Position, joint2.Position };
      pt = depth.ConvertRealWorldToProjective(pt);

      Graphics g = Graphics.FromImage(bitmap);
      g.DrawLine(pen, pt[0].X, pt[0].Y, pt[1].X, pt[1].Y);
    }
  }
}
