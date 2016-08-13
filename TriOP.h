/*
 * TriOP.h
 *
 *  Created on: Jul 20, 2016
 *      Author: mason
 */

#ifndef TRIOP_H_
#define TRIOP_H_

#include "Param.h"
#include "MyLib.h"

struct TriParam {
public:
	Param _W1;
	Param _W2;
	Param _W3;
	Param _b;

	bool _bUseB;

public:
	TriParam() {
		_bUseB = true;
	}

	inline void exportAdaParams(ModelUpdate& ada) {
		ada.addParam(&_W1);
		ada.addParam(&_W2);
		ada.addParam(&_W3);
		if (_bUseB) {
			ada.addParam(&_b);
		}
	}

	inline void initial(int nOSize, int nISize1, int nISize2, int nISize3, bool bUseB = true, int seed = 0) {
		srand(seed);
		_W1.initial(nOSize, nISize1);
		_W2.initial(nOSize, nISize2);
		_W3.initial(nOSize, nISize3);
		_b.initial(nOSize, 1);

		_bUseB = bUseB;
	}

};

// non-linear feed-forward node
// input nodes should be specified by forward function
// for input variables, we exploit column vector,
// which means a concrete input vector x_i is represented by x(0, i), x(1, i), ..., x(n, i)
struct TriNode {
public:
	PMat _px1, _px2, _px3;
	Mat _y, _ly;
	Mat _ty, _lty;  // t means temp, _ty is to save temp vector before activation

	int _inDim1, _inDim2, _inDim3, _outDim;

	TriParam* _param;

	Mat (*_f)(const Mat&);   // activation function
	Mat (*_f_deri)(const Mat&, const Mat&);  // derivation function of activation function

public:
	TriNode() {
	}

	TriNode(TriParam* param) {
		_px1 = NULL;
		_px2 = NULL;
		_px3 = NULL;
		_f = tanh;
		_f_deri = tanh_deri;
		_y.setZero();
		_ly.setZero();
		_ty.setZero();
		_lty.setZero();
		setParam(param);
	}

	TriNode(Mat (*f)(const Mat&), Mat (*f_deri)(const Mat&, const Mat&)) {
		_px1 = NULL;
		_px2 = NULL;
		_px3 = NULL;
		setFunctions(f, f_deri);
		_y.setZero();
		_ly.setZero();
		_ty.setZero();
		_lty.setZero();
		_param = NULL;
		_inDim1 = 0;
		_inDim2 = 0;
		_inDim3 = 0;
		_outDim = 0;
	}

	TriNode(TriParam* param, Mat (*f)(const Mat&), Mat (*f_deri)(const Mat&, const Mat&)) {
		_px1 = NULL;
		_px2 = NULL;
		_px3 = NULL;
		setFunctions(f, f_deri);
		_y.setZero();
		_ly.setZero();
		_ty.setZero();
		_lty.setZero();
		setParam(param);
	}

	inline void setParam(TriParam* param) {
		_param = param;
		_inDim1 = _param->_W1.inDim();
		_inDim2 = _param->_W2.inDim();
		_inDim3 = _param->_W3.inDim();
		_outDim = _param->_W1.outDim();
		if (!_param->_bUseB) {
			cout << "please check whether _bUseB is true, usually this should be true for non-linear layer" << endl;
		}
	}

	inline void clear(){
		_px1 = NULL;
		_px2 = NULL;
		_px3 = NULL;
		_f = tanh;
		_f_deri = tanh_deri;
		_y.setZero();
		_ly.setZero();
		_ty.setZero();
		_lty.setZero();
		_param = NULL;
		_inDim1 = 0;
		_inDim2 = 0;
		_inDim3 = 0;
		_outDim = 0;
	}

	inline void clearValue(){
		_px1 = NULL;
		_px2 = NULL;
		_px3 = NULL;
		_y.setZero();
		_ly.setZero();
		_ty.setZero();
		_lty.setZero();
	}

	// define the activation function and its derivation form
	inline void setFunctions(Mat (*f)(const Mat&), Mat (*f_deri)(const Mat&, const Mat&)) {
		_f = f;
		_f_deri = f_deri;
	}

public:
	void forward(PMat px1, PMat px2, PMat px3) {
		assert(_param != NULL);

		_ty = _param->_W1.val * (*px1) + _param->_W2.val * (*px2) + _param->_W3.val * (*px3);
		if(_param->_bUseB){
			for (int idx = 0; idx < _ty.cols(); idx++) {
				_ty.col(idx) += _param->_b.val.col(0);
			}
		}

		_y = _f(_ty);
		_px1 = px1;
		_px2 = px2;
		_px2 = px3;
	}

	void backward(PMat plx1, PMat plx2, PMat plx3) {
		assert(_param != NULL);

		_lty = _ly.array() * _f_deri(_ty, _y).array();

		_param->_W1.grad += _lty * _px1->transpose();
		_param->_W2.grad += _lty * _px2->transpose();
		_param->_W3.grad += _lty * _px3->transpose();

		if(_param->_bUseB){
			for (int idx = 0; idx < _y.cols(); idx++) {
				_param->_b.grad.col(0) += _lty.col(idx);
			}
		}

		if (plx1->size() == 0) {
			*plx1 = Mat::Zero(_px1->rows(), _px1->cols());
		}

		if (plx2->size() == 0) {
			*plx2 = Mat::Zero(_px2->rows(), _px2->cols());
		}

		if (plx3->size() == 0) {
			*plx3 = Mat::Zero(_px3->rows(), _px3->cols());
		}

		*plx1 += _param->_W1.val.transpose() * _lty;
		*plx2 += _param->_W2.val.transpose() * _lty;
		*plx3 += _param->_W3.val.transpose() * _lty;
	}

};

#endif /* TRIOP_H_ */