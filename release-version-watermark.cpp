#include <opencv2/opencv.hpp>
#include <istream>
#include <stdio.h>
#include <string>
#include <vector>
#include <math.h>
#include <ctime>        // std::time
#include <cstdlib>      // std::rand, std::srand
#include <algorithm>
#include <fstream>

using namespace std;
using namespace cv;
/**************************************functions**********************************************/
//from vector to Mat
template<typename T=uchar>
inline cv::Mat idecoupage(const std::vector<cv::Mat_<T>>& vBlocks, const cv::Size& oImageSize, int nChannels) {
	std::vector<cv::Mat_<T>> mergedMatrix(nChannels);
	cv::Mat outputImage;
	int nbBlocbycols = oImageSize.width / 8; // nombre de bloc par colonne
	int nbBlocbyrows = oImageSize.height / 8; // nombre de bloc par ligne

	for (int c = 0; c<nChannels; ++c)
	{
		cv::Mat t1;
		std::vector<cv::Mat> mt1;
		for (int i = 0; i<nbBlocbyrows; ++i) 
		{
			cv::Mat t2;
			std::vector<cv::Mat> mt2;
			for (int j = 0; j<nbBlocbycols; ++j) 
			{
				mt2.push_back(vBlocks[c*nbBlocbycols *nbBlocbyrows + i*nbBlocbycols + j]);		// Ajout du bloc dans le vecteur	
			}
			cv::hconcat(mt2,t2); // Concatenation de la matrice avec le vecteur (Ajout par colonne)
			mt1.push_back(t2); // Ajout du vecteur dans la matrice
			
		}
		cv::vconcat(mt1, t1); // Concatenation de la matrice avec le vecteur (Ajout par ligne)
		mergedMatrix[c].push_back(t1); // Ajout du vecteur dans la matrice finale
	}
	cv::merge(mergedMatrix, outputImage); // Transforme en matrice RGB
	return outputImage;
}
//from Mat to vector
template<typename T=uchar>
inline std::vector<cv::Mat_<T>> decoupage(const cv::Mat& oImage) 
{
	CV_Assert(oImage.depth()==CV_8U);

	int nbBlocbycols = oImage.cols/ 8; // nombre de bloc par colonne
	int nbBlocbyrows = oImage.rows / 8;// nombre de bloc par ligne
	std::vector<cv::Mat_<T>> blocMat(nbBlocbycols * nbBlocbyrows *oImage.channels());
	std::vector<cv::Mat_<T>> rgbChannels(oImage.channels());
	
	if (oImage.channels() == 1)
	{
		rgbChannels[0] = oImage;
	}
	else
	{
		cv::split(oImage, rgbChannels);
	}

	for (int c = 0; c < rgbChannels.size(); ++c)
	{
		for (int i = 0; i<nbBlocbyrows; ++i) 
		{
			for (int j = 0; j<nbBlocbycols; ++j) 
			{
				blocMat[c*nbBlocbyrows *nbBlocbycols + i * nbBlocbycols + j] = rgbChannels[c].cv::Mat::colRange(j * 8, (j + 1) * 8).cv::Mat::rowRange(i * 8, (i + 1) * 8);
			}
		}
	}

	return blocMat;
}
//zigzag scan from mat to vector
template<int nBlockSize = 8>
inline std::vector<int> zigzag(const cv::Mat_<float>& mat) 
{
    CV_Assert(!mat.empty());
    CV_Assert(mat.rows==mat.cols && mat.rows==nBlockSize);
	int nIdx = 0;
	std::vector<int> zigzag(nBlockSize*nBlockSize);
	
	for (int i = 0; i < nBlockSize * 2; ++i)
		for (int j = (i < nBlockSize) ? 0 : i - nBlockSize + 1; j <= i && j < nBlockSize; ++j)
			zigzag[nIdx++] = mat((i & 1) ? j*(nBlockSize - 1) + i : (i - j)*nBlockSize + j);

	return zigzag;
}
//izigzag from vector to mat
template<int nBlockSize=8,typename T=float>
inline cv::Mat_<T> izigzag(const std::vector<T>& vec) 
{
    CV_Assert(!vec.empty());
    CV_Assert(int(vec.size())==nBlockSize*nBlockSize);
    int nIdx = 0;
    cv::Mat_<T> oMatResult(nBlockSize*nBlockSize,1);
    for(int i=0; i<nBlockSize*2; ++i)
        for(int j=(i<nBlockSize) ? 0 : i-nBlockSize+1; j<=i && j<nBlockSize; ++j)
            oMatResult((i&1) ? j*(nBlockSize-1)+i :(i-j)*nBlockSize+j) = vec[nIdx++];
    return oMatResult.reshape(0,nBlockSize);
}
//generate a ramdom map
int myrandom (int i) { return std::rand()%i;}
inline vector<int> generator(int a,int b)
{
  ofstream myfile ("pseu-random sequence map.txt");
  srand ( unsigned ( std::time(0) ) );
  vector<int> myvector;
  for (int i=0; i<a*b; ++i) myvector.push_back(i);
  random_shuffle ( myvector.begin(), myvector.end() );
  random_shuffle ( myvector.begin(), myvector.end(), myrandom);
  if (myfile.is_open())
  {
    for(int i=0; i<myvector.size(); ++i)
    {
      myfile << myvector[i] << ' ';
    }
    myfile.close();
  }
  return myvector;
}
// calculate SMRE, PSNR
float calRms(Mat originIMG, Mat wmIMG)
{
	float e;
	unsigned a = 0;
	int m = originIMG.rows, n = originIMG.cols;
	for (int i = 0; i < m; i++)
	{
		for (int j = 0; j < n; j++)
		{
			a = a + pow((wmIMG.at<uchar>(i, j) - originIMG.at<uchar>(i, j)), 2);
		}
	}
	e = pow(a / m / n, 0.5);
	return e;
}

float calSnr(Mat originIMG, Mat wmIMG)
{
	float s;
	unsigned a = 0, b = 0;
	int m = originIMG.rows, n = originIMG.cols;
	for (int i = 0; i < m; i++)
	{
		for (int j = 0; j < n; j++)
		{
			a = a + pow((wmIMG.at<uchar>(i, j) - originIMG.at<uchar>(i, j)), 2);
			b = b + pow(wmIMG.at<uchar>(i, j), 2);
		}
	}
	s = b / a;
	return s;
}

/***************************************process watermark********************************************************/
vector<int> convertTo_binary(char src[100]){
	Mat img = imread(src,0);
	vector<int> convert;
	int blocks = img.cols/8; // blocks means number of block in cols
	vector<Mat_<uchar>> vectorBlock = decoupage(img);
	Mat create(8,8,CV_8UC1,Scalar::all(0));
	vector<Mat_<uchar>> scramvectorBlock(blocks*blocks,create);
	vector<int> generate = generator(blocks,blocks);
	for (int i = 0; i < blocks*blocks; ++i)
	{
		int index = generate[i];
		scramvectorBlock[i] = vectorBlock[index];
	}
	Mat wmIMG = idecoupage(scramvectorBlock,img.size(), img.channels());
	namedWindow("after",WINDOW_AUTOSIZE);
	imshow("after",wmIMG);
	imwrite("scramble.jpg",wmIMG);
	waitKey(0);
	for (int i = 0; i < wmIMG.rows; ++i)
	{	
		for (int j = 0; j < wmIMG.cols; ++j)
		{
			if (wmIMG.at<uchar>(i,j) > 100)
			{
				convert.push_back(1);
			}else
			convert.push_back(0);
		}
	}
	return convert;
}
/******************************************** MAIN ********************************************/
int main()
{	
	std::vector<int> vectorwm;
	vectorwm = convertTo_binary("BKHN-2.jpg");
	Mat originIMG = imread("lena.jpg", 0);
	if (originIMG.empty())
	{
		cout << "error!" << endl;
		return -1;
	}
	Mat imgcopy = originIMG.clone();
	float wmscalor = 1;// the watermark scalor
	int a = originIMG.rows /8 +1;
	int b = originIMG.cols /8 +1;
	int blocks = (originIMG.rows/8)*(originIMG.cols/8);
	Mat wmIMG;
	int flag=0;
	//make border of watermark img
	copyMakeBorder(originIMG, wmIMG, 0, 8*a - originIMG.rows, 0, 8*b - originIMG.cols, BORDER_CONSTANT, Scalar::all(0));
	//read per block
	for (int i = 0;flag < vectorwm.size(); i++)
	{
		for (int j = 0; j <b-1; j++)
		{
			Mat img(8, 8, CV_32FC1);
			//read 1 block
			if (i<a-1)
			{
				for (int r = i * 8, f = 0; r < i * 8 + 8, f < 8; r++, f++)
				{
					for (int t = j * 8, g = 0; t < j * 8 + 8, g < 8; t++, g++)
					{
						img.at<float>(f, g) = wmIMG.at<uchar>(r, t);
					}
				}
			}else{
				for (int r = (i%(a-1)) * 8, f = 0; r < (i%(a-1)) * 8 + 8, f < 8; r++, f++)
				{
					for (int t = j * 8, g = 0; t < j * 8 + 8, g < 8; t++, g++)
					{
						img.at<float>(f, g) = wmIMG.at<uchar>(r, t);
					}
				}	
			}
			dct(img-128, img); //dct
			//Quantum with K =90
			Mat_<int> Matquantum(8,8,CV_32FC1);
			Mat Q = (Mat_<float>(8,8)<<  3,2,2,3,5,8,10,12,2,2,3,4,5,12,12,11,3,3,3,5,8,11,14,11,3,3,4,6,10,17,16,12,4,4,7,11,14,22,21,15,5,7,11,13,16,12,23,18,10,13,16,17,21,24,24,21,14,18,19,20,22,20,20,20);				
			for (int i = 0; i < 8; ++i)
			{	
				for (int j = 0; j < 8; ++j)
				{
					Matquantum.at<int>(i,j) = round(img.at<float>(i,j)/Q.at<float>(i,j));
				}
			}
			//zigzag scan 
			vector<int> v1;
			v1 = zigzag(Matquantum);
			vector<float> v(64,0);
			for (int i = 0; i < 64; ++i)
			{
				v[i] = v1[i];
			}
			//embeded watermark
			int x=6+2*(flag/blocks),y = 7+2*(flag/blocks);
				if(vectorwm.at(flag)==1)
				{
					if (v[x]>v[y])
					{
						swap(v[x],v[y]);
						
					}
					if (v[x]==v[y])
					{
						v[y]=+wmscalor;
					}
				}
				if(vectorwm.at(flag)==0)
				{
					if(v[x]<v[y])
					{
						swap(v[x],v[y]);
					}
					if (v[x]==v[y])
					{
						v[x]=v[x]+wmscalor;
					}
				}
			//izigzag
			Mat multiquantum;
			multiquantum.create(8,8,CV_32FC1);
			Mat temp;
			temp.create(8,8,CV_32FC1);
			temp = izigzag(v);
			//dequantum
			for (int i = 0; i < 8; ++i)
			{	
				for (int j = 0; j < 8; ++j)
				{
					multiquantum.at<float>(i,j) = temp.at<float>(i,j) * Q.at<float>(i,j);
				}
			}
			idct(multiquantum, img);//idct
			//big 128
			if (i<a-1)
			{
				for (int r = i * 8, f = 0; r < i * 8 + 8, f < 8; r++, f++)
				{
					for (int t = j * 8, g = 0; t < j * 8 + 8, g < 8; t++, g++)
					{
						wmIMG.at<uchar>(r, t) = img.at<float>(f, g) +128;
					}
				}
			}else{
				for (int r = (i%(a-1)) * 8, f = 0; r < (i%(a-1)) * 8 + 8, f < 8; r++, f++)
				{
					for (int t = j * 8, g = 0; t < j * 8 + 8, g < 8; t++, g++)
					{
						wmIMG.at<uchar>(r, t) = img.at<float>(f, g) +128;
					}
				}	
			}
			flag++;
		}
	}
	// watermarkIMG with true size
	for (int u = 0; u < imgcopy.rows; u++)
	{
		for (int v = 0; v < imgcopy.cols; v++)
		{
			imgcopy.at<uchar>(u, v) = wmIMG.at<uchar>(u, v);
		}
	}
	Mat img5 = imread("lena.jpg", 0);
	imwrite("watermark.jpg",imgcopy);
	float e = calRms(img5, wmIMG);
	float s = calSnr(img5, wmIMG);
	cout << "**********" << endl;
	cout << "RMSE=" << e << endl;
	cout << "SNR=" << s << endl;
	cout << "**********" << endl;
	imshow("source image", img5);
	imshow("watermark", imgcopy);
	waitKey();
	return 0;
}
