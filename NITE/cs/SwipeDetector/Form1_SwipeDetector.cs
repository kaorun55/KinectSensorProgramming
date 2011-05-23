﻿using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;

namespace SwipeDetector
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

    private xn.Context context;
    private xn.ImageGenerator image;

    private xnv.SessionManager sessionManager;
    private xnv.SwipeDetector swipeDetector;

    enum SessionState
    {
      NotInSession,
      DetectSession,
      InSession,
    }

    private SessionState sessionState = SessionState.NotInSession;
    private xnv.Direction direction = xnv.Direction.Illegal;

    // 描画用
    private Brush brush = new SolidBrush(Color.Black);
    private Font font = new Font("Arial", 30);
    private PointF point = new Point(0, 0);

    // 初期化
    private void xnInitialize()
    {
      // コンテキストの初期化
      context = new xn.Context(CONFIG_XML_PATH);

      // イメージジェネレータの作成
      image = context.FindExistingNode(xn.NodeType.Image) as xn.ImageGenerator;
      if (image == null) {
        throw new Exception(context.GetGlobalErrorState());
      }

      // NITEのためのセッションマネージャを作成
      sessionManager = new xnv.SessionManager(context, "Wave,Click", "RaiseHand");

      // セッションの開始と終了を通知するコールバックを登録する
      sessionManager.SessionStart +=
            new xnv.SessionManager.SessionStartHandler(sessionManager_SessionStart);
      sessionManager.SessionEnd +=
            new xnv.SessionManager.SessionEndHandler(sessionManager_SessionEnd);
      sessionManager.SessionFocusProgress +=
            new xnv.SessionManager.SessionFocusProgressHandler(
                                                sessionManager_SessionFocusProgress);

      // Wave(左右運動の検出器)
      swipeDetector = new xnv.SwipeDetector();
      swipeDetector.GeneralSwipe +=
            new xnv.SwipeDetector.GeneralSwipeHandler(swipeDetector_GeneralSwipe);
      swipeDetector.SwipeUp +=
            new xnv.SwipeDetector.SwipeUpHandler(swipeDetector_SwipeUp);
      swipeDetector.SwipeDown +=
            new xnv.SwipeDetector.SwipeDownHandler(swipeDetector_SwipeDown);
      swipeDetector.SwipeRight +=
            new xnv.SwipeDetector.SwipeRightHandler(swipeDetector_SwipeRight);
      swipeDetector.SwipeLeft +=
            new xnv.SwipeDetector.SwipeLeftHandler(swipeDetector_SwipeLeft);

      // リスナーに追加する
      sessionManager.AddListener(swipeDetector);

      // ジェネレータの動作を開始する
      context.StartGeneratingAll();
    }

    // 描画
    private unsafe void xnDraw()
    {
      // コンテキストとセッションマネージャーの更新
      context.WaitAndUpdateAll();
      sessionManager.Update(context);

      // 画像データを取得する
      xn.ImageMetaData imageMD = image.GetMetaData();

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


        // 現在の状態を表示する
        Graphics g = Graphics.FromImage(bitmap);
        string message = sessionState.ToString() + "\r\n" +
                         direction.ToString();
        g.DrawString(message, font, brush, new Point(0, 0));
      }
    }

    // キーイベント
    private void xnKeyDown(Keys key)
    {
    }

    // 左方向への動きを通知する
    void swipeDetector_SwipeLeft(float velocity, float angle)
    {
      brush = new SolidBrush(Color.Red);
    }

    // 右方向への動きを通知する
    void swipeDetector_SwipeRight(float velocity, float angle)
    {
      brush = new SolidBrush(Color.Blue);
    }

    // 下方向への動きを通知する
    void swipeDetector_SwipeDown(float velocity, float angle)
    {
      brush = new SolidBrush(Color.Green);
    }

    // 上方向への動きを通知する
    void swipeDetector_SwipeUp(float velocity, float angle)
    {
      brush = new SolidBrush(Color.Yellow);
    }

    // スワイプの検出を通知する
    void swipeDetector_GeneralSwipe(xnv.Direction dir, float velocity, float angle)
    {
      direction = dir;
    }

    // セッションの開始を通知する
    void sessionManager_SessionStart(ref xn.Point3D position)
    {
      sessionState = SessionState.InSession;
    }

    // セッションの終了を通知する
    void sessionManager_SessionEnd()
    {
      sessionState = SessionState.NotInSession;
    }

    // セッションフォーカスの検出を通知する
    void sessionManager_SessionFocusProgress(string strFocus,
                                        ref xn.Point3D ptPosition, float fProgress)
    {
      sessionState = SessionState.DetectSession;
    }
  }
}
