using System;
using System.Windows.Forms;

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
                xn.Context context = new xn.Context();
                MessageBox.Show("Success");
            }
            catch (Exception ex) {
                MessageBox.Show(ex.Message);
            }
        }
    }
}
