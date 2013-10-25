#ifndef _CANVAS2D_SAVER_H_
#define _CANVAS2D_SAVER_H_

#include <iostream>
#include <string>
#include "SsMotion.h"
#include "TextEncoding.h"

/**
 * HTML5/Canvas2Dプレイヤー形式で出力する
 */
class Canvas2dSaver
{
public:

	/** 出力オプション */
	struct Options
	{
		bool	isJson;
		bool	isNoSuffix;
	};

	/** constructor */
	Canvas2dSaver(std::ostream& out, textenc::Encoding outEncoding, const Options& options);

	/** destructor */
	~Canvas2dSaver();


	/** 
	 * ImageListあるとき画像リストを出力 
	 */
	void writeImageList(
		ss::SsMotion::Ptr motion,
		const std::string& prefixLabel
		);

	/**
	 * 画像リストを出力 
	 */
	void writeImageList(
		ss::SsImageList::ConstPtr imageList,
		const std::string& prefixLabel
		);

	/**
	 * アニメーションデータを出力 
	 */
	void writeAnimation(
		ss::SsMotion::Ptr motion,
		ss::SsImageList::ConstPtr optImageList,
		const std::string& prefixLabel
		);


private:
	void prewriteBlock();

	std::ostream&		_out;
	textenc::Encoding	_outEncoding;
	const Options		_options;
	bool				_blockWritten;
};

#endif	// _CANVAS2D_SAVER_H_
