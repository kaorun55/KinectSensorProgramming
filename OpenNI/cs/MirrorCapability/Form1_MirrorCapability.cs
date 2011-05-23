using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;
using System.Collections.Generic;

namespace MirrorCapability
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

    private xn.Context context;
    private xn.ImageGenerator image;
    private xn.DepthGenerator depth;

    private Dictionary<string, bool> mirrorState = new Dictionary<string,bool>();

    private int[] histogram;

    // 描画用
    private Brush brush = new SolidBrush(Color.Black);
    private Font font = new Font("Arial", 30);
    private PointF point = new PointF(0, 0);

    // 初期化
    private void xnInitialize()
    {
      // コンテキストの初期化
      context = new xn.Context(CONFIG_XML_PATH);

      // イメージジェネレータの作成
      image = context.FindExistingNode(xn.NodeType.Image)
                                              as xn.ImageGenerator;
      if (image == null) {
        throw new Exception(context.GetGlobalErrorState());
      }

      // デプスジェネレータの作成
      depth = context.FindExistingNode(xn.NodeType.Depth)
                                              as xn.DepthGenerator;
      if (depth == null) {
        throw new Exception(context.GetGlobalErrorState());
      }

      // デプスの座標をイメージに合わせる
      depth.GetAlternativeViewPointCap().SetViewPoint(image);

      // カメラ画像の
      //   ミラー状態が変更されたことを通知するコールバックを登録
      //   ミラー状態の取得
      xn.MirrorCapability imageMirror = image.GetMirrorCap();
      imageMirror.MirrorChangedEvent += new xn.StateChangedHandler(
                                                    Form1_MirrorChangedEvent);
      mirrorState.Add(image.ToString(), imageMirror.IsMirrored());


      // デプスの
      //   ミラー状態が変更されたことを通知するコールバックを登録
      //   ミラー状態の取得
      xn.MirrorCapability depthMirror = depth.GetMirrorCap();
      depthMirror.MirrorChangedEvent += new xn.StateChangedHandler(
                                                    Form1_MirrorChangedEvent);
      mirrorState.Add(depth.ToString(), depthMirror.IsMirrored());

      // ヒストグラムバッファの作成
      histogram = new int[depth.GetDeviceMaxDepth()];
    }

    // ミラー状態が変化したときに通知する
    void Form1_MirrorChangedEvent(xn.ProductionNode node)
    {
      xn.Generator generator = node as xn.Generator;
      if (generator != null) {
        mirrorState[generator.ToString()] = generator.GetMirrorCap().IsMirrored();
      }
    }

    // 描画
    private unsafe void xnDraw()
    {
      // ノードの更新を待ち、データを取得する
      context.WaitAndUpdateAll();
      xn.ImageMetaData imageMD = image.GetMetaData();
      xn.DepthMetaData depthMD = depth.GetMetaData();

      CalcHist(depthMD);

      // カメラ画像の作成
      lock (this) {
        // 書き込み用のビットマップデータを作成
        Rectangle rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        BitmapData data = bitmap.LockBits(rect, ImageLockMode.WriteOnly,
                                                PixelFormat.Format24bppRgb);

        // 生データへのポインタを取得
        byte* dst = (byte*)data.Scan0.ToPointer();
        byte* src = (byte*)image.GetImageMapPtr().ToPointer();
        ushort* dep = (ushort*)depth.GetDepthMapPtr().ToPointer();

        for (int i = 0; i < imageMD.DataSize; i += 3, src += 3, dst += 3, ++dep) {
          byte pixel = (byte)histogram[*dep];
          // ヒストグラムの対象外の場合、カメライメージを描画する
          if (pixel == 0) {
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
          }
          // それ以外の場所はヒストラムを描画する
          else {
            dst[0] = 0;
            dst[1] = pixel;
            dst[2] = pixel;
          }
        }

        bitmap.UnlockBits(data);


        // 現在の状態を表示する
        Graphics g = Graphics.FromImage(bitmap);
        string message = "";
        message += "ImageMirror:" + mirrorState[image.ToString()] + "\n";
        message += "DepthMirror:" + mirrorState[depth.ToString()];
        g.DrawString(message, font, brush, point);
      }
    }

    // キーイベント
    private void xnKeyDown(Keys key)
    {
      // すべてを反転する
      if (key == Keys.M) {
        context.SetGlobalMirror(!context.GetGlobalMirror());
      }
      // イメージのみ反転する
      else if (key == Keys.I) {
        xn.MirrorCapability mirror = image.GetMirrorCap();
        mirror.SetMirror(!mirror.IsMirrored());
      }
      // デプスのみ反転する
      else if (key == Keys.D) {
        xn.MirrorCapability mirror = depth.GetMirrorCap();
        mirror.SetMirror(!mirror.IsMirrored());
      }
    }

    // ヒストグラムの計算
    private unsafe void CalcHist(xn.DepthMetaData depthMD)
    {
      for (int i = 0; i < histogram.Length; ++i) {
        histogram[i] = 0;
      }

      ushort* pDepth = (ushort*)depthMD.DepthMapPtr.ToPointer();

      int points = 0;
      for (int y = 0; y < depthMD.YRes; ++y) {
        for (int x = 0; x < depthMD.XRes; ++x, ++pDepth) {
          ushort depthVal = *pDepth;
          if (depthVal != 0) {
            histogram[depthVal]++;
            points++;
          }
        }
      }

      for (int i = 1; i < histogram.Length; i++) {
        histogram[i] += histogram[i - 1];
      }

      if (points > 0) {
        for (int i = 1; i < histogram.Length; i++) {
          histogram[i] = (int)(256 * (1.0f - (histogram[i] / (float)points)));
        }
      }
    }
  }
}
