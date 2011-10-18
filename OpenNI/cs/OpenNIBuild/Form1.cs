using System;
using System.Windows.Forms;
using OpenNI;

namespace OpenNIBuild
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            try {
                Context context = new Context();
                MessageBox.Show( "Success" );
            }
            catch (Exception ex) {
                MessageBox.Show(ex.Message);
            }
        }
    }
}
