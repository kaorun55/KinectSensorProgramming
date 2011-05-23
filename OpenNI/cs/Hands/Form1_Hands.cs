using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;
using System.Collections.Generic;

namespace Hands
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

    private xn.Context context;
    private xn.ImageGenerator image;
    private xn.DepthGenerator depth;
    private xn.GestureGenerator gesture;
    private xn.HandsGenerator hands;

    private Queue<xn.Point3D> handPoints = new Queue<xn.Point3D>();
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

      // ジェスチャージェネレータの作成
      gesture = context.FindExistingNode(xn.NodeType.Gesture) as xn.GestureGenerator;
      if (depth == null) {
        throw new Exception(context.GetGlobalErrorState());
      }

      // ジェスチャーの登録
      gesture.AddGesture("RaiseHand");

      // ジェスチャー用のコールバックを登録
      gesture.GestureRecognized += new xn.GestureGenerator.GestureRecognizedHandler(
                                                        gesture_GestureRecognized);
      gesture.GestureProgress += new xn.GestureGenerator.GestureProgressHandler(
                                                        gesture_GestureProgress);

      // ハンドジェネレータの作成
      hands = context.FindExistingNode(xn.NodeType.Hands) as xn.HandsGenerator;
      if (depth == null) {
        throw new Exception(context.GetGlobalErrorState());
      }

      // ハンドトラッキング用のコールバックを登録する
      hands.HandCreate += new xn.HandsGenerator.HandCreateHandler(hands_HandCreate);
      hands.HandUpdate += new xn.HandsGenerator.HandUpdateHandler(hands_HandUpdate);
      hands.HandDestroy += new xn.HandsGenerator.HandDestroyHandler(
                                                                hands_HandDestroy);

      // ジェスチャーの検出開始
      context.StartGeneratingAll();
    }

    // 描画
    private unsafe void xnDraw()
    {
      // カメライメージの更新を待ち、画像データを取得する
      context.WaitOneUpdateAll(image);
      xn.ImageMetaData imageMD = image.GetMetaData();

      Graphics g;

      // カメラ画像の作成
      lock (this) {
        // 書き込み用のビットマップデータを作成
        Rectangle rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        BitmapData data = bitmap.LockBits(rect, ImageLockMode.WriteOnly,
                        System.Drawing.Imaging.PixelFormat.Format24bppRgb);

        // 生データへのポインタを取得
        byte* dst = (byte*)data.Scan0.ToPointer();
        byte* src = (byte*)image.GetImageMapPtr().ToPointer();

        for (int i = 0; i < imageMD.DataSize; i += 3, src += 3, dst += 3) {
          dst[0] = src[2];
          dst[1] = src[1];
          dst[2] = src[0];
        }

        bitmap.UnlockBits(data);

        // 手の軌跡を描画
        if (handPoints.Count != 0) {
          xn.Point3D start = depth.ConvertRealWorldToProjective(handPoints.Peek());
          foreach (xn.Point3D handPoint in handPoints) {
            xn.Point3D pt = depth.ConvertRealWorldToProjective(handPoint);
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
    void gesture_GestureProgress(xn.ProductionNode node, string strGesture,
                                        ref xn.Point3D position, float progress)
    {
      gestureStatus = GestureStatus.Progress;
    }

    // ジェスチャーを検出した
    void gesture_GestureRecognized(xn.ProductionNode node, string strGesture,
                          ref xn.Point3D idPosition, ref xn.Point3D endPosition)
    {
      gestureStatus = GestureStatus.Recognized;

      // 手のトラッキングを開始する
      hands.StartTracking(ref endPosition);
    }

    // 手の検出開始
    void hands_HandCreate(xn.ProductionNode node, uint id,
                                ref xn.Point3D position, float fTime)
    {
      handStates = HandStatus.Create;
    }

    // 手の位置の更新
    void hands_HandUpdate(xn.ProductionNode node, uint id,
                                ref xn.Point3D position, float fTime)
    {
      handStates = HandStatus.Update;
      handPoints.Enqueue(position);
    }

    // 手の検出終了
    void hands_HandDestroy(xn.ProductionNode node, uint id, float fTime)
    {
      handStates = HandStatus.NoTracking;
      handPoints.Clear();

      // トラッキングの停止
      hands.StopTracking(id);
    }
  }
}
