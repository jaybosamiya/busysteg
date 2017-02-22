#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace cv;
using namespace std;

char* progpath;

void usage() {
  cerr << "Usage: \n";
  cerr << "  " << progpath << " h <image path> <data path> <output image path>\n";
  cerr << "  " << progpath << " x <image path> <output data path>\n";
}

void fatalerror(const char* error) {
  cerr << "ERROR: " << error << endl;
  usage();
  exit(1);
}

void info(const char* msg) {
  cerr << "[+] " << msg << endl;
}

void hide_data(char* inimg, char* indata, char* outimg);
void extract_data(char *inimg, char* outdata);

int main( int argc, char** argv ) {
  progpath = argv[0];
  if ( argc < 2 ) {
    fatalerror("No arguments passed");
  }
  if ( argv[1][1] != '\0' ) {
    fatalerror("Operation must be a single letter");
  }
  if ( argv[1][0] == 'h' ) {
    if ( argc != 5 ) {
      fatalerror("Wrong number of parameters for [h]ide operation");
    }
    hide_data(argv[2], argv[3], argv[4]);
  } else if ( argv[1][0] == 'x' ) {
    if ( argc != 4 ) {
      fatalerror("Wrong number of parameters for e[x]tract operation");
    }
    extract_data(argv[2], argv[3]);
  } else {
    fatalerror("Unknown operation");
  }
  return 0;
}

Mat calc_energy(Mat img) {
  Mat orig;
  Mat shifted;
  Mat diff;
  Mat res;

  bitwise_and(img, 0xF0, img);

  copyMakeBorder(img, orig, 1, 1, 1, 1, BORDER_REPLICATE);

  res = Mat::zeros(orig.size(), orig.type());

  int top[8] = {1,0,0,0,1,2,2,2};
  int left[8] = {2,2,1,0,0,0,1,2};
  for ( int i = 0 ; i < 8 ; i++ ) {
    copyMakeBorder(img, shifted, top[i], 2-top[i], left[i], 2-left[i], BORDER_REPLICATE);
    absdiff(orig, shifted, diff);
    res = max(res, diff);
  }

  return res(Rect(1, 1, img.cols, img.rows)); // x, y, width, height
}

typedef pair<pair<uchar, int>, pair<int, pair<int, int> > > Energy;

inline Energy _energy(int r, int c, int ch, uchar v) {
  int nonce = ch * ch * 10666589 + r * r + c * c + 2239; // to "uniformly" distribute data
  return make_pair(make_pair(v, nonce), make_pair(ch, make_pair(c, r)));
}

inline int _energy_r(const Energy &e) { return e.second.second.second; }
inline int _energy_c(const Energy &e) { return e.second.second.first; }
inline int _energy_ch(const Energy &e) { return e.second.first; }
inline int _energy_v(const Energy &e) { return e.first.first; }

vector<Energy> energy_order(Mat img) {
  /* Returns a vector in decreasing order of energy. */

  Mat energy = calc_energy(img.clone());

  info("Calculated energies");

  vector<Energy> energylist;

  for ( int r = 0 ; r < img.rows ; r++ ) {
    for ( int c = 0 ; c < img.cols ; c++ ) {
      const Vec3b vals = energy.at<Vec3b>(r,c);
      for ( int ch = 0 ; ch < 3 ; ch++ ) {
	uchar v = vals[ch];
	if ( v > 0 ) {
	  energylist.push_back(_energy(r,c,ch,v));
	}
      }
    }
  }

  sort(energylist.begin(), energylist.end());
  reverse(energylist.begin(), energylist.end());

  return energylist;
}

void write_into(Mat &img, vector<Energy> pts, char *buf, int size) {
  int written = 0;
  char val;
  int count = 0;
  for ( vector<Energy>::iterator it = pts.begin() ;
	it != pts.end() && written != size ;
	it++, count++ ) {
    uchar data;
    if ( count % 2 == 0 ) {
      val = buf[written];
      data = (val & 0xf0) / 0x10;
    } else {
      data = (val & 0x0f);
      written += 1;
    }

    Energy &e = *it;
    Vec3b &vals = img.at<Vec3b>(_energy_r(e), _energy_c(e));
    uchar &v = vals[_energy_ch(e)];
    v = (0xf0 & v) + data;
  }

  if ( written != size ) {
    fatalerror("Could not write all bytes");
  }
}

void read_from(Mat &img, vector<Energy> pts, char* buf, int size) {
  int read = 0;

  int count = 0;
  char val = 0;

  for ( vector<Energy>::iterator it = pts.begin() ;
	it != pts.end() && read != size ;
	it++, count++ ) {
    Energy &e = *it;
    const Vec3b val = img.at<Vec3b>(_energy_r(e), _energy_c(e));
    const uchar v = val[_energy_ch(e)];
    const uchar data = 0x0f & v;
    char out;
    if ( count % 2 == 0 ) {
      out = data * 0x10;
    } else {
      out += data;
      buf[read++] = out;
    }
  }

  if ( read != size ) {
    fatalerror("Wrong size");
  }
}

void hide_data(char* inimg, char* indata, char* outimg) {
  Mat img = imread(inimg, CV_LOAD_IMAGE_COLOR);
  if ( ! img.data ) {
    fatalerror("Could not load image. Please check path.");
  }
  info("Loaded image");

  ifstream fin(indata, ios_base::binary);
  long int fsize = fin.tellg();
  fin.seekg(0, ios_base::end);
  fsize = fin.tellg() - fsize;
  fin.seekg(0, ios_base::beg);
  char *buf = new char[fsize + 16];
  memcpy(buf, "BUSYSTEG", 8);
  memcpy(buf + 8, &fsize, 8);
  fin.read(buf + 16, fsize);
  fin.close();
  info("Read data");

  vector<Energy> pts = energy_order(img);
  info("Found energy ordering");

  write_into(img, pts, buf, fsize + 16);
  info("Updated pixel values");

  imwrite(outimg, img);
  info("Finished writing image");

  delete[] buf;
}

void extract_data(char *inimg, char* outdata) {
  Mat img = imread(inimg, CV_LOAD_IMAGE_COLOR);
  if ( ! img.data ) {
    fatalerror("Could not load image. Please check path.");
  }
  info("Loaded image");

  vector<Energy> pts = energy_order(img);
  info("Found energy ordering");

  char header[16];
  read_from(img, pts, header, 16);
  if ( memcmp(header, "BUSYSTEG", 8) != 0 ) {
    fatalerror("Not a busysteg encoded image");
  }

  long int fsize;
  memcpy(&fsize, header+8, 8);
  info("Found data length");

  char *buf = new char[fsize + 16];
  read_from(img, pts, buf, fsize + 16);
  info("Loaded data from pixels");

  ofstream fout(outdata, ios_base::binary);
  fout.write(buf + 16, fsize);
  fout.close();
  info("Finished writing data");

  delete [] buf;
}
