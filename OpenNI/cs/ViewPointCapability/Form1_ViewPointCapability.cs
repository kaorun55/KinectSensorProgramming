using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;
using OpenNI;

namespace ViewpointCapability
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

    private Context context;
    private ImageGenerator image;
    private DepthGenerator depth;

    private int[] histogram;

    private bool isViewpoint;

    // 描画用
    private Brush brush = new SolidBrush(Color.Black);
    private Font font = new Font("Arial", 30);

    // 初期化
    private void xnInitialize()
    {
      // コンテキストの初期化
      context = new Context(CONFIG_XML_PATH);

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

      // ビューポイントが変更されたことを通知するコールバックを登録する
      AlternativeViewpointCapability Viewpoint =
                            depth.AlternativeViewpointCapability;
      Viewpoint.ViewpointChanged += new EventHandler(Viewpoint_ViewpointChanged);

      // ビューポイントのサポート状態を確認する
      if (!Viewpoint.IsViewpointSupported(image)) {
        throw new Exception("ビューポイントをサポートしていません");
      }

      // 現在の状態を取得する
      isViewpoint = Viewpoint.IsViewpointAs(image);

      // ヒストグラムバッファの作成
      histogram = new int[depth.DeviceMaxDepth];
    }

    void Viewpoint_ViewpointChanged(object sender, EventArgs e)
    {
      throw new NotImplementedException();
    }

    // ビューポイントが変化したことを通知する
    void Viewpoint_ViewpointChanged(ProductionNode node)
    {
      DepthGenerator depth = node as DepthGenerator;
      if (depth != null) {
        isViewpoint = depth.AlternativeViewpointCapability.IsViewpointAs(image);
      }
    }

    // 描画
    private unsafe void xnDraw()
    {
      // ノードの更新を待ち、データを取得する
      context.WaitAndUpdateAll();
      ImageMetaData imageMD = image.GetMetaData();
      DepthMetaData depthMD = depth.GetMetaData();

      CalcHist(depthMD);

      // カメラ画像の作成
      lock (this) {
        // 書き込み用のビットマップデータを作成
        Rectangle rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        BitmapData data = bitmap.LockBits(rect, ImageLockMode.WriteOnly,
                             System.Drawing.Imaging.PixelFormat.Format24bppRgb);

        // 生データへのポインタを取得
        byte* dst = (byte*)data.Scan0.ToPointer();
        byte* src = (byte*)image.ImageMapPtr.ToPointer();
        ushort* dep = (ushort*)depth.DepthMapPtr.ToPointer();

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
        string message = "ImageViewpoint:" + isViewpoint;
        g.DrawString(message, font, brush, new PointF(0, 0));
      }
    }

    // キーイベント
    private void xnKeyDown(Keys key)
    {
      // ビューポイントの設定を変更する
      if (key == Keys.V) {
        AlternativeViewpointCapability Viewpoint =
                                    depth.AlternativeViewpointCapability;
        // ビューポイントがイメージにセットされている場合は、リセットする
        if (Viewpoint.IsViewpointAs(image)) {
          Viewpoint.ResetViewpoint();
        }
        // ビューポイントがイメージにセットされていない場合は、イメージをセットする
        else {
          Viewpoint.SetViewpoint(image);
        }
      }
    }

    // ヒストグラムの計算
    private unsafe void CalcHist(DepthMetaData depthMD)
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
