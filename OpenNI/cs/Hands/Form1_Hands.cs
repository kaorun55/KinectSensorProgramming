using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;
using System.Collections.Generic;
using OpenNI;

namespace Hands
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

    private Context context;
    private ImageGenerator image;
    private DepthGenerator depth;
    private GestureGenerator gesture;
    private HandsGenerator hands;

    private Queue<Point3D> handPoints = new Queue<Point3D>();
    private const int MAX_POINT = 30;

    // ジェスチャーの状態
    enum GestureStatus
    {
      Unrecognize,    // 未検出
      Progress,       // 検出中
      Recognized,     // 検出した
    }

    private GestureStatus gestureStatus = GestureStatus.Unrecognize;

    // ハンドトラッキングの状態
    enum HandStatus
    {
      NoTracking, // トラッキングしてない
      Create,     // トラッキングの開始
      Update,     // トラッキングデータの更新
    }

    private HandStatus handStates = HandStatus.NoTracking;


    // 描画用
    private Pen pen = new Pen(Color.Red, 5);
    private Brush brush = new SolidBrush(Color.Black);
    private Font font = new Font("Arial", 30);
    private PointF point = new PointF(0, 0);

    // 初期化
    private void xnInitialize()
    {
      // コンテキストの初期化
      context = new Context(CONFIG_XML_PATH);

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

      // ジェスチャージェネレータの作成
      gesture = context.FindExistingNode(NodeType.Gesture) as GestureGenerator;
      if (depth == null) {
        throw new Exception(context.GlobalErrorState);
      }

      // ジェスチャーの登録
      gesture.AddGesture("RaiseHand");

      // ジェスチャー用のコールバックを登録
      gesture.GestureRecognized += new EventHandler<GestureRecognizedEventArgs>(gesture_GestureRecognized);
      gesture.GestureProgress += new EventHandler<GestureProgressEventArgs>(gesture_GestureProgress);

      // ハンドジェネレータの作成
      hands = context.FindExistingNode(NodeType.Hands) as HandsGenerator;
      if (depth == null) {
        throw new Exception(context.GlobalErrorState);
      }

      // ハンドトラッキング用のコールバックを登録する
      hands.HandCreate += new EventHandler<HandCreateEventArgs>(hands_HandCreate);
      hands.HandUpdate += new EventHandler<HandUpdateEventArgs>(hands_HandUpdate);
      hands.HandDestroy += new EventHandler<HandDestroyEventArgs>(hands_HandDestroy);

      // ジェスチャーの検出開始
      context.StartGeneratingAll();
    }

    // 描画
    private unsafe void xnDraw()
    {
      // カメライメージの更新を待ち、画像データを取得する
      context.WaitOneUpdateAll(image);
      ImageMetaData imageMD = image.GetMetaData();

      Graphics g;

      // カメラ画像の作成
      lock (this) {
        // 書き込み用のビットマップデータを作成
        Rectangle rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        BitmapData data = bitmap.LockBits(rect, ImageLockMode.WriteOnly,
                        System.Drawing.Imaging.PixelFormat.Format24bppRgb);

        // 生データへのポインタを取得
        byte* dst = (byte*)data.Scan0.ToPointer();
        byte* src = (byte*)image.ImageMapPtr.ToPointer();

        for (int i = 0; i < imageMD.DataSize; i += 3, src += 3, dst += 3) {
          dst[0] = src[2];
          dst[1] = src[1];
          dst[2] = src[0];
        }

        bitmap.UnlockBits(data);

        // 手の軌跡を描画
        if (handPoints.Count != 0) {
          Point3D start = depth.ConvertRealWorldToProjective(handPoints.Peek());
          foreach (Point3D handPoint in handPoints) {
            Point3D pt = depth.ConvertRealWorldToProjective(handPoint);
            g = Graphics.FromImage(bitmap);
            g.DrawLine(pen, start.X, start.Y, pt.X, pt.Y);
            start = pt;
          }

          // 先頭要素を削除
          if (handPoints.Count > MAX_POINT) {
            handPoints.Dequeue();
          }
        }
      }


      // 現在の状態を表示する
      g = Graphics.FromImage(bitmap);
      string message = "Gesture:RaiseHand" +
                      ", Status:" + gestureStatus.ToString() + "\n" +
                      "Hand:" + handStates.ToString();
      g.DrawString(message, font, brush, point);
    }

    // キーイベント
    private void xnKeyDown(Keys key)
    {
    }

    // ジェスチャーの検出中
    void gesture_GestureProgress(object sender, GestureProgressEventArgs e)
    {
      gestureStatus = GestureStatus.Progress;
    }

    // ジェスチャーを検出した
    void gesture_GestureRecognized(object sender, GestureRecognizedEventArgs e)
    {
      gestureStatus = GestureStatus.Recognized;

      // 手のトラッキングを開始する
      hands.StartTracking(e.EndPosition);
    }

    // 手の検出開始
    void hands_HandCreate(object sender, HandCreateEventArgs e)
    {
      handStates = HandStatus.Create;
    }

    // 手の位置の更新
    void hands_HandUpdate(object sender, HandUpdateEventArgs e)
    {
      handStates = HandStatus.Update;
      handPoints.Enqueue(e.Position);
    }

    // 手の検出終了
    void hands_HandDestroy(object sender, HandDestroyEventArgs e)
    {
      // 1.1.0.41 ではHandGenerator.StopTrackingでHandDestroyイベントが
      // 呼び出されるようで再帰になってしまう。
      // そのためトラッキング停止状態でない場合のみ、トラッキング停止をする
      if (handStates != HandStatus.NoTracking) {
        handStates = HandStatus.NoTracking;
        handPoints.Clear();

        // トラッキングの停止
        hands.StopTracking(e.UserID);
      }
    }
  }
}
