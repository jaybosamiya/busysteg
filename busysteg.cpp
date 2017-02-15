#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <fstream>

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

void hide_data(char* inimg, char* indata, char* outimg) {
  // TODO
}

void extract_data(char *inimg, char* outdata) {
  // TODO
}
