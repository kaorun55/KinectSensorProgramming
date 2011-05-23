using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;

namespace PoseDetect
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

    private xn.Context context;
    private xn.ImageGenerator image;
    private xn.DepthGenerator depth;
    private xn.UserGenerator user;
    private xn.SkeletonCapability skelton;

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
      context = new xn.Context(CONFIG_XML_PATH);

      // 鏡モード(反転)にしない
      context.SetGlobalMirror(false);

      // イメージジェネレータの作成
      image = context.FindExistingNode(xn.NodeType.Image) as xn.ImageGenerator;
      if (image == null) {
        throw new Exception(context.GetGlobalErrorState());
      }

      // デプスジェネレータの作成
      depth = context.FindExistingNode(xn.NodeType.Depth) as xn.DepthGenerator;
      if (depth == null) {
        throw new Exception(context.GetGlobalErrorState());
      }

      // デプスの座標をイメージに合わせる
      depth.GetAlternativeViewPointCap().SetViewPoint(image);

      // ユーザージェネレータの作成
      user = context.FindExistingNode(xn.NodeType.User) as xn.UserGenerator;
      if (depth == null) {
        throw new Exception(context.GetGlobalErrorState());
      }

      // ユーザー検出機能をサポートしているか確認
      if (!user.IsCapabilitySupported("User::Skeleton")) {
        throw new Exception("ユーザー検出をサポートしていません");
      }

      // ユーザー認識のコールバックを登録
      user.NewUser += new xn.UserGenerator.NewUserHandler(user_NewUser);
      user.LostUser += new xn.UserGenerator.LostUserHandler(user_LostUser);

      //キャリブレーションにポーズが必要か確認
      skelton = user.GetSkeletonCap();
      if (skelton.NeedPoseForCalibration()) {
        // ポーズ検出のサポートチェック
        if (!user.IsCapabilitySupported("User::PoseDetection")) {
          throw new Exception("ユーザー検出をサポートしていません");
        }

        // キャリブレーションポーズの取得
        pose = skelton.GetCalibrationPose();

        // ポーズ検出のコールバックを登録
        xn.PoseDetectionCapability poseDetect = user.GetPoseDetectionCap();
        poseDetect.PoseDetected +=
            new xn.PoseDetectionCapability.PoseDetectedHandler(
                                          poseDetect_PoseDetected);
        poseDetect.PoseEnded +=
            new xn.PoseDetectionCapability.PoseEndedHandler(
                                          poseDetect_PoseEnded);
      }

      // キャリブレーションのコールバックを登録
      skelton.CalibrationStart +=
            new xn.SkeletonCapability.CalibrationStartHandler(
                                        skelton_CalibrationStart);
      skelton.CalibrationEnd +=
            new xn.SkeletonCapability.CalibrationEndHandler(
                                        skelton_CalibrationEnd);

      // すべてをトラッキングする
      skelton.SetSkeletonProfile(xn.SkeletonProfile.All);

      // ジェスチャーの検出開始
      context.StartGeneratingAll();
    }

    // 描画
    private unsafe void xnDraw()
    {
      // カメライメージの更新を待ち、画像データを取得する
      context.WaitOneUpdateAll(image);
      xn.ImageMetaData imageMD = image.GetMetaData();
      xn.SceneMetaData sceneMD = user.GetUserPixels(0);

      // カメラ画像の作成
      lock (this) {
        // 書き込み用のビットマップデータを作成
        Rectangle rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        BitmapData data = bitmap.LockBits(rect, ImageLockMode.WriteOnly,
                        System.Drawing.Imaging.PixelFormat.Format24bppRgb);

        // 生データへのポインタを取得
        byte* dst = (byte*)data.Scan0.ToPointer();
        byte* src = (byte*)image.GetImageMapPtr().ToPointer();
        ushort* label = (ushort*)sceneMD.SceneMapPtr.ToPointer();

        for (int i = 0; i < imageMD.DataSize; i += 3, src += 3, dst += 3, ++label) {
          dst[0] = (byte)(src[2] * colors[*label, 0]);
          dst[1] = (byte)(src[1] * colors[*label, 1]);
          dst[2] = (byte)(src[0] * colors[*label, 2]);
        }

        bitmap.UnlockBits(data);

        // スケルトンの描画
        foreach (uint id in user.GetUsers()) {
          // トラッキング対象のユーザーでなければ次へ
          if (!user.GetSkeletonCap().IsTracking(id)) {
            continue;
          }

          // スケルトンを描画する
          DrawSkeleton(id);

          // 腕の交点を描画する
          DrawCrossPoint(id);
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
    void user_NewUser(xn.ProductionNode node, uint id)
    {
      message = "ユーザー検出:" + id;

      // ポーズの検出が必要であれば、ポーズの検出を開始する
      if (!string.IsNullOrEmpty(pose)) {
        user.GetPoseDetectionCap().StartPoseDetection(pose, id);
      }
      // ポーズの検出が不要であれば、キャリブレーションを開始する
      else {
        user.GetSkeletonCap().RequestCalibration(id, true);
      }
    }

    // ユーザー消失
    void user_LostUser(xn.ProductionNode node, uint id)
    {
      message = "ユーザー消失:" + id;
    }

    // ポーズ検出
    void poseDetect_PoseDetected(xn.ProductionNode node, string pose, uint id)
    {
      message = "ポーズ検出:" + pose + " ユーザー:" + id;

      // ポーズの検出を停止し、キャリブレーションを開始する
      user.GetPoseDetectionCap().StopPoseDetection(id);
      user.GetSkeletonCap().RequestCalibration(id, true);
    }

    // ポーズ検出終了
    void poseDetect_PoseEnded(xn.ProductionNode node, string pose, uint id)
    {
      message = "ポーズ消失:" + pose + " ユーザー:" + id;
    }

    // キャリブレーション開始
    void skelton_CalibrationStart(xn.ProductionNode node, uint id)
    {
      message = "キャリブレーション開始:" + id;
    }

    // キャリブレーション終了
    void skelton_CalibrationEnd(xn.ProductionNode node, uint id, bool success)
    {
      // キャリブレーション成功
      if (success) {
        message = "キャリブレーション成功:" + id;
        user.GetSkeletonCap().StartTracking(id);
      }
      // キャリブレーション失敗
      else {
        message = "キャリブレーション失敗:" + id;
      }
    }

    // パーツの座標を取得する
    xn.SkeletonJointPosition GetJointPosition(uint player, xn.SkeletonJoint eJoint1)
    {
      xn.SkeletonJointPosition joint = new xn.SkeletonJointPosition();
      skelton.GetSkeletonJointPosition(player, eJoint1, ref joint);
      return joint;
    }

    // 骨格の線を引く
    void DrawLine(uint player, xn.SkeletonJoint eJoint1, xn.SkeletonJoint eJoint2)
    {
      // 各箇所の座標を取得する
      xn.SkeletonJointPosition joint1 = GetJointPosition(player, eJoint1);
      xn.SkeletonJointPosition joint2 = GetJointPosition(player, eJoint2);
      if (joint1.fConfidence < 0.5 || joint2.fConfidence < 0.5) {
        return;
      }

      // 現実の座標から画面の座標に変換する
      xn.Point3D[] pt = new xn.Point3D[] { joint1.position, joint2.position };
      pt = depth.ConvertRealWorldToProjective(pt);

      Graphics g = Graphics.FromImage(bitmap);
      g.DrawLine(pen, pt[0].X, pt[0].Y, pt[1].X, pt[1].Y);
    }

    // スケルトンを描画する
    private void DrawSkeleton(uint player)
    {
      DrawLine(player, xn.SkeletonJoint.Head, xn.SkeletonJoint.Neck);

      DrawLine(player, xn.SkeletonJoint.Neck, xn.SkeletonJoint.LeftShoulder);
      DrawLine(player, xn.SkeletonJoint.LeftShoulder, xn.SkeletonJoint.LeftElbow);
      DrawLine(player, xn.SkeletonJoint.LeftElbow, xn.SkeletonJoint.LeftHand);

      DrawLine(player, xn.SkeletonJoint.Neck, xn.SkeletonJoint.RightShoulder);
      DrawLine(player, xn.SkeletonJoint.RightShoulder, xn.SkeletonJoint.RightElbow);
      DrawLine(player, xn.SkeletonJoint.RightElbow, xn.SkeletonJoint.RightHand);

      DrawLine(player, xn.SkeletonJoint.LeftShoulder, xn.SkeletonJoint.Torso);
      DrawLine(player, xn.SkeletonJoint.RightShoulder, xn.SkeletonJoint.Torso);

      DrawLine(player, xn.SkeletonJoint.Torso, xn.SkeletonJoint.LeftHip);
      DrawLine(player, xn.SkeletonJoint.LeftHip, xn.SkeletonJoint.LeftKnee);
      DrawLine(player, xn.SkeletonJoint.LeftKnee, xn.SkeletonJoint.LeftFoot);

      DrawLine(player, xn.SkeletonJoint.Torso, xn.SkeletonJoint.RightHip);
      DrawLine(player, xn.SkeletonJoint.RightHip, xn.SkeletonJoint.RightKnee);
      DrawLine(player, xn.SkeletonJoint.RightKnee, xn.SkeletonJoint.RightFoot);

      DrawLine(player, xn.SkeletonJoint.LeftHip, xn.SkeletonJoint.RightHip);
    }

    // 腕の交差点を描画する
    private void DrawCrossPoint(uint player)
    {
      xn.Point3D[] parts = new xn.Point3D[4] {
        GetJointPosition(player, xn.SkeletonJoint.LeftElbow).position,
        GetJointPosition(player, xn.SkeletonJoint.LeftHand).position,
        GetJointPosition(player, xn.SkeletonJoint.RightElbow).position,
        GetJointPosition(player, xn.SkeletonJoint.RightHand).position,
      };

      // 腕が交差している場合に、交差している座標を表示する
      bool isCross = CrossHitCheck(parts[0], parts[1], parts[2], parts[3]);
      if (isCross) {
        parts = depth.ConvertRealWorldToProjective(parts);
        xn.Point3D point = GetCrossPoint(parts[0], parts[1], parts[2], parts[3]);

        Graphics g = Graphics.FromImage(bitmap);
        g.DrawEllipse(new Pen(Color.Red, 10), 
          new Rectangle(new Point((int)point.X, (int)point.Y), new Size(10, 10)));
      }
    }

    // こちらのページを参考にしました
    // http://net2.cocolog-nifty.com/blog/2009/11/post-8792.html
    bool CrossHitCheck(xn.Point3D a1, xn.Point3D a2, xn.Point3D b1, xn.Point3D b2)
    {
      // 外積:axb = ax*by - ay*bx
      // 外積と使用して交差判定を行なう
      double v1 = (a2.X - a1.X) * (b1.Y - a1.Y) - (a2.Y - a1.Y) * (b1.X - a1.X);
      double v2 = (a2.X - a1.X) * (b2.Y - a1.Y) - (a2.Y - a1.Y) * (b2.X - a1.X);
      double m1 = (b2.X - b1.X) * (a1.Y - b1.Y) - (b2.Y - b1.Y) * (a1.X - b1.X);
      double m2 = (b2.X - b1.X) * (a2.Y - b1.Y) - (b2.Y - b1.Y) * (a2.X - b1.X);
      // +-,-+だったら-値になるのでそれぞれを掛けて確認する
      if ((v1 * v2 <= 0) && (m1 * m2 <= 0)) {
        return true; // ２つとも左右にあった
      }
      else {
        return false;
      }
    }

    // こちらのページを参考にしました
    // http://net2.cocolog-nifty.com/blog/2009/11/post-dbea.html
    xn.Point3D GetCrossPoint(xn.Point3D a1, xn.Point3D a2, xn.Point3D b1, xn.Point3D b2)
    {
      xn.Point3D tmp = new xn.Point3D();// 計算用
      // １つめの式
      float v1a = (a1.Y - a2.Y) / (a1.X - a2.X);
      float v1b = (a1.X * a2.Y - a1.Y * a2.X) / (a1.X - a2.X);
      // ２つめの式
      float v2a = (b1.Y - b2.Y) / (b1.X - b2.X);
      float v2b = (b1.X * b2.Y - b1.Y * b2.X) / (b1.X - b2.X);
      // 最終的な交点
      tmp.X = (v2b - v1b) / (v1a - v2a);
      tmp.Y = v1a * tmp.X + v1b;
      return tmp;// x,yの答えを返す
    }
  }
}
