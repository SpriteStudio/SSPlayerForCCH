
#include "SSPlayer.h"
#include "SSPlayerData.h"
#include <cstring>
#include <string>

using namespace cocos2d;

/*BatchNode使用改変説明

例えば次のようなパーツを持つアニメーションデータについて、
 0 : a.png
 1 : a.png
 2 : a.png
 3 : b.png
 4 : b.png
 5 : a.png
 6 : b.png
 
 SSPlayerに次のような構造のオブジェクトを生成し、描画を行う
 
 SSPlayer
  ├0:CCSpriteBatchNode(テクスチャa.pngをターゲット）
  │　├0:CCSprite(0番目のパーツを描画）
  │　├1:CCSprite(1番目のパーツを描画）
  │　└2:CCSprite(2番目のパーツを描画）
  ├1:CCSpriteBatchNode(テクスチャb.pngをターゲット）
  │　├0:CCSprite(3番目のパーツを描画）
  │　└1:CCSprite(4番目のパーツを描画）
  └2:CCSpriteBatchNode(テクスチャa.pngをターゲット）
  	　├0:CCSprite(5番目のパーツを描画）
  	　└1:CCSprite(6番目のパーツを描画）
 
 0-2番目、3-4番目、5-6番目はそれぞれBatchNodeでまとめて描画されるため、
 高速に描画することが可能になる。
 0-2番目を描画しているバッチノードで5-6番目のテクスチャを描画してしまうと、
 表示順序が変わってしまい正しく表示できなくなってしまうため、
 別々のBatchNodeが必要になる。
 
 別々のテクスチャを使用したパーツが交互に重なるアニメーションの場合効果は無いが、
 同じテクスチャを使用しているパーツには効果大となる。
 
**/


// 独自Spriteクラスの使用可否定義
// 本プレイヤーでは、カラーブレンドと頂点変形を実装するためCCSpriteを継承した一部独自実装のクラスを使用しています。
// 将来、CCSpriteの仕様変更によりコンパイルが通らなくなるなど不都合が起こったときのため、
// この定義で使用可否を切り替えられるようにしています。
// ※独自Spriteを使用しない場合、カラーブレンドと頂点変形の効果は反映されなくなります。
//
// Availability own definition of the custom sprite class.
// In this player,
// I use some classes in their own implementation that inherits from CCSprite
// to implement the 'Color Blend' and 'Vertex Deformation'.
// For when inconvenience occurs future,
// compile and will not pass through the specification change of CCSprite,
// I have to switch the availability in this definition.
// If not use is 'Color Blend' and 'Vertex Deformation' not reflected.

#define USE_CUSTOM_SPRITE		1		// (0:Not use, 1:Use)


// ContentScaleFactorに合わせてUVを調整します
// 有効にする場合、USE_CUSTOM_SPRITEも1である必要があります.
#define ADJUST_UV_BY_CONTENT_SCALE_FACTOR	0	// (0:disable, 1:enable)



/**
 * definition
 */

static const ss_u32 SSDATA_ID_0 = 0xffffffff;
static const ss_u32 SSDATA_ID_1 = 0x53534241;
static const ss_u32 SSDATA_VERSION = 5;



/**
 * SSPlayerHelper
 */

/** ssbaファイルをロードします.
 *  使用済みポインタは必ず delete[] で破棄してください.
 *  Load ssba file.
 *  A pointer used, must be discard with delete[].
 */
unsigned char* SSPlayerHelper::loadFile(const char* ssbaPath, const char* dir)
{
	CCAssert(ssbaPath != NULL, "SSPlayerHelper::loadFile: Invalid argument.");

	std::string path;
	if (dir) path.append(dir);
	path.append(ssbaPath);
#if (COCOS2D_VERSION >= 0x00020100)
	std::string fullpath = CCFileUtils::sharedFileUtils()->fullPathForFilename(path.c_str());
#else
	std::string fullpath = CCFileUtils::sharedFileUtils()->fullPathFromRelativePath(path.c_str());
#endif
	unsigned long nSize = 0;
	unsigned char* data = CCFileUtils::sharedFileUtils()->getFileData(fullpath.c_str(), "rb", &nSize);
	return data;
}

/** ssbaファイルからSSPlayer/SSImageListオブジェクトを構築します
 *  outDataに返されるポインタの破棄には必ず delete[] を使用してください.
 *  Create SSPlayer/SSImageList objects, from ssba file.
 *  outData returned pointer used, must be discard with delete[].
 */
void SSPlayerHelper::createFromFile(unsigned char** outData, SSPlayer** outPlayer, SSImageList** outImageList, const char* ssbaPath, const char* dir)
{
	CCAssert(
		outData != NULL &&
		outPlayer != NULL &&
		outImageList != NULL &&
		ssbaPath != NULL,
		"SSPlayerHelper::createFromFile: Invalid argument.");

	unsigned char* data = loadFile(ssbaPath, dir);
	*outData = data;
	
	SSData* ssdata = reinterpret_cast<SSData*>(data);

	*outImageList = SSImageList::create(ssdata, dir);
	*outPlayer = SSPlayer::create(ssdata, *outImageList);
}



/**
 * SSDataHandle
 */

class SSDataHandle
{
public:
	SSDataHandle(const SSData* data)
		: m_data(data)
	{
		CCAssert(data->id[0] == SSDATA_ID_0, "Not id 0 matched.");
		CCAssert(data->id[1] == SSDATA_ID_1, "Not id 1 matched.");
		CCAssert(data->version == SSDATA_VERSION, "Version number of data does not match.");
	}
	
	const SSData* getData() const { return m_data; }
	
	ss_u32 getFlags() const { return m_data->flags; }
	int getNumParts() const { return m_data->numParts; }
	int getNumFrames() const { return m_data->numFrames; }
	int getFps() const { return m_data->fps; }

	const SSPartData* getPartData() const
	{
		return static_cast<const SSPartData*>(getAddress(m_data->partData));
	}

	const SSFrameData* getFrameData() const
	{
		return static_cast<const SSFrameData*>(getAddress(m_data->frameData));
	}
	
	const ss_offset* getImageData() const
	{
		return static_cast<const ss_offset*>(getAddress(m_data->imageData));
	}
	
	const void* getAddress(ss_offset offset) const
	{
		return static_cast<const void*>( reinterpret_cast<const char*>(m_data) + offset );
	}
	
	const char* getPartName(int partId) const
	{
		CCAssert(partId >= 0 && partId < m_data->numParts, "partId is out of range.");
	
		const SSPartData* partData = getPartData();
		const char* name = static_cast<const char*>(getAddress(partData[partId].name));
		return name;
	}

	int indexOfPart(const char* partName) const
	{
		for (int i = 0; i < m_data->numParts; i++)
		{
			const char* name = getPartName(i);
			if (std::strcmp(partName, name) == 0)
			{
				return i;
			}
		}
		return -1;
	}

private:
	const SSData*	m_data;
};



/**
 * SSDataReader
 */

class SSDataReader
{
public:
	SSDataReader(const ss_u16* dataPtr)
		: m_dataPtr(dataPtr)
	{}

	ss_u16 readU16() { return *m_dataPtr++; }
	ss_s16 readS16() { return static_cast<ss_s16>(*m_dataPtr++); }

	unsigned int readU32()
	{
		unsigned int l = readU16();
		unsigned int u = readU16();
		return static_cast<unsigned int>((u << 16) | l);
	}

	int readS32()
	{
		return static_cast<int>(readU32());
	}

	float readFloat()
	{
		union {
			float			f;
			unsigned int	i;
		} c;
		c.i = readU32();
		return c.f;
	}
	
	void readColor(ccColor4B& color)
	{
		unsigned int raw = readU32();
		color.a = static_cast<GLubyte>(raw >> 24);
		color.r = static_cast<GLubyte>(raw >> 16);
		color.g = static_cast<GLubyte>(raw >> 8);
		color.b = static_cast<GLubyte>(raw);
	}
	
	const char* getString(int* length)
	{
		int len = readU16();
		const char* str = reinterpret_cast<const char*>(m_dataPtr);
		
		int skip = ((len+1) + 1) >> 1;
		m_dataPtr += skip;
		
		*length = len;
		return str;
	}

private:
	const ss_u16*	m_dataPtr;
};



/**
 * SSImageList
 */

SSImageList::SSImageList(void)
{
}

SSImageList::~SSImageList()
{
	removeAll();
}

SSImageList* SSImageList::create()
{
	SSImageList* imageList = new SSImageList();
	imageList->autorelease();
	return imageList;
}

SSImageList* SSImageList::create(const SSData* ssData, const char* imageDir)
{
	CCAssert(ssData != NULL, "zero is pointer");

	SSImageList* imageList = new SSImageList();
	if (imageList && imageList->init(ssData, imageDir))
	{
		imageList->autorelease();
		return imageList;
	}
	CC_SAFE_DELETE(imageList);
	return NULL;
}

SSImageList* SSImageList::create(const char* imageFilenames[], const char* imageDir)
{
	CCAssert(imageFilenames != NULL, "zero is imageFilenames pointer");

	SSImageList* imageList = new SSImageList();
	if (imageList && imageList->init(imageFilenames, imageDir))
	{
		imageList->autorelease();
		return imageList;
	}
	CC_SAFE_DELETE(imageList);
	return NULL;
}

bool SSImageList::init(const SSData* ssData, const char* imageDir)
{
	CCAssert(ssData != NULL, "zero is pointer");

	removeAll();
	
	SSDataHandle dataHandle(ssData);
	const ss_offset* imageData = dataHandle.getImageData();

	for (size_t i = 0; imageData[i] != 0; i++)
	{
		const char* imageName = static_cast<const char*>(dataHandle.getAddress(imageData[i]));
		addTexture(imageName, imageDir);
	}

	return true;
}

bool SSImageList::init(const char* imageFilenames[], const char* imageDir)
{
	CCAssert(imageFilenames != NULL, "zero is imageFilenames pointer");

	removeAll();

	for (size_t i = 0; imageFilenames[i] != 0; i++)
	{
		addTexture(imageFilenames[i], imageDir);
	}

	return true;
}

void SSImageList::removeAll()
{
	CCTextureCache* texCache = CCTextureCache::sharedTextureCache();
	for (size_t i = 0, count = m_imageList.count(); i < count; i++)
	{
		CCTexture2D* tex = static_cast<CCTexture2D*>( m_imageList.objectAtIndex(i) );
		texCache->removeTexture(tex);
	}

	m_imageList.removeAllObjects();
}

CCTexture2D* SSImageList::getTexture(size_t index)
{
	if (index >= m_imageList.count()) return NULL;

	CCTexture2D* tex = static_cast<CCTexture2D*>( m_imageList.objectAtIndex(index) );
	return tex;
}

void SSImageList::addTexture(const char* imageName, const char* imageDir)
{
	std::string path = s_generator(imageName, imageDir);
	
	CCTextureCache* texCache = CCTextureCache::sharedTextureCache();
	CCTexture2D* tex = texCache->addImage(path.c_str());
	if (tex == NULL)
	{
		CCLOG("image load failed: %s", path.c_str());
		CC_ASSERT(0);
	}
	CCLOG("Load image: %s", path.c_str());
	m_imageList.addObject(tex);
}

std::string SSImageList::defaultImagePathGenerator(const char* imageName, const char* imageDir)
{
	std::string path;
	if (imageDir)
	{
		path.append(imageDir);
		size_t pathLen = path.length();
		if (pathLen && path.at(pathLen-1) != '/' && path.at(pathLen-1) != '\\')
		{
			path.append("/");
		}
	}
	path.append(imageName);
	return path;
}

/*
static std::string exampleImagePathGenerator(const char* imageName, const char* imageDir)
{
	std::string path;
	if (imageDir)
	{
		path.append(imageDir);
		size_t pathLen = path.length();
		if (pathLen && path.at(pathLen-1) != '/' && path.at(pathLen-1) != '\\')
		{
			path.append("/");
		}
	}

	float csf = CCDirector::sharedDirector()->getContentScaleFactor();
	// ContentScaleFactorの値により読み込み先ディレクトリを変更する
	if (csf >= 2.0f)
	{
		path.append("hd/");
	}
	else
	{
		path.append("sd/");
	}

	path.append(imageName);
	return path;
}
*/

SSImageList::ImagePathGenerator SSImageList::s_generator = SSImageList::defaultImagePathGenerator;

void SSImageList::setImagePathGenerator(ImagePathGenerator generator)
{
	s_generator = generator;
}





/**
 * SSPartState
 */

class SSPartState : public cocos2d::CCObject
{
public:
	SSPartState();
	virtual ~SSPartState();

	void init();
	void copyParameters(SSPlayer::PartState& state) const;	

private:
	friend class SSPlayer;

	float	m_x;
	float	m_y;
	float	m_scaleX;
	float	m_scaleY;
	float	m_rotation;
	cocos2d::CCSprite*	m_sprite;
	CCAffineTransform	m_trans;
};

SSPartState::SSPartState()
{
	this->autorelease();
	init();
}

SSPartState::~SSPartState()
{
}

void SSPartState::init()
{
	m_x = 0;
	m_y = 0;
	m_scaleX = 1.0f;
	m_scaleY = 1.0f;
	m_rotation = 0.0f;
	m_sprite = NULL;
	m_trans = CCAffineTransformMakeIdentity();
}

void SSPartState::copyParameters(SSPlayer::PartState& state) const
{
	state.x = m_x;
	state.y = m_y;
	state.scaleX = m_scaleX;
	state.scaleY = m_scaleY;
	state.rotation = m_rotation;
	state.sprite = m_sprite;
}



#if USE_CUSTOM_SPRITE

/**
 * SSSprite
 */

class SSSprite : public CCSprite
{
private:
	static unsigned int ssSelectorLocation;
	static unsigned int	ssAlphaLocation;

	static CCGLProgram* getCustomShaderProgram();

private:
	CCGLProgram*	_defaultShaderProgram;
	bool			_useCustomShaderProgram;
	float			_opacity;
	int				_colorBlendFuncNo;

public:
	SSSprite();
	virtual ~SSSprite();

	static SSSprite* create();

	// override
	virtual void draw(void);
	virtual void setOpacity(GLubyte opacity);

#if ADJUST_UV_BY_CONTENT_SCALE_FACTOR
	virtual void setVertexRect(const CCRect& rect);
#endif
	
	// original functions
	void changeShaderProgram(bool useCustomShaderProgram);
	bool isCustomShaderProgramEnabled() const;
	void setColorBlendFunc(int colorBlendFuncNo);
	ccV3F_C4B_T2F_Quad& getAttributeRef();
};

#endif	// if USE_CUSTOM_SPRITE



/**
 * flags definition
 */

enum {
	SS_DATA_FLAG_USE_VERTEX_OFFSET	= 1 << 0,
	SS_DATA_FLAG_USE_COLOR_BLEND	= 1 << 1,
	SS_DATA_FLAG_USE_ALPHA_BLEND	= 1 << 2,
	SS_DATA_FLAG_USE_AFFINE_TRANS	= 1 << 3,

	NUM_SS_DATA_FLAGS
};

enum {
	SS_PART_FLAG_FLIP_H				= 1 << 0,
	SS_PART_FLAG_FLIP_V				= 1 << 1,
	SS_PART_FLAG_INVISIBLE			= 1 << 2,
	
	SS_PART_FLAG_ORIGIN_X			= 1 << 4,
	SS_PART_FLAG_ORIGIN_Y			= 1 << 5,
	SS_PART_FLAG_ROTATION			= 1 << 6,
	SS_PART_FLAG_SCALE_X			= 1 << 7,
	SS_PART_FLAG_SCALE_Y			= 1 << 8,
	SS_PART_FLAG_OPACITY			= 1 << 9,
	SS_PART_FLAG_VERTEX_OFFSET_TL	= 1 << 10,
	SS_PART_FLAG_VERTEX_OFFSET_TR	= 1 << 11,
	SS_PART_FLAG_VERTEX_OFFSET_BL	= 1 << 12,
	SS_PART_FLAG_VERTEX_OFFSET_BR	= 1 << 13,
	SS_PART_FLAG_COLOR				= 1 << 14,
	SS_PART_FLAG_VERTEX_COLOR_TL	= 1 << 15,
	SS_PART_FLAG_VERTEX_COLOR_TR	= 1 << 16,
	SS_PART_FLAG_VERTEX_COLOR_BL	= 1 << 17,
	SS_PART_FLAG_VERTEX_COLOR_BR	= 1 << 18,

	SS_PART_FLAGS_VERTEX_OFFSET		= SS_PART_FLAG_VERTEX_OFFSET_TL |
									  SS_PART_FLAG_VERTEX_OFFSET_TR |
									  SS_PART_FLAG_VERTEX_OFFSET_BL |
									  SS_PART_FLAG_VERTEX_OFFSET_BR,

	SS_PART_FLAGS_COLOR_BLEND		= SS_PART_FLAG_COLOR |
									  SS_PART_FLAG_VERTEX_COLOR_TL |
									  SS_PART_FLAG_VERTEX_COLOR_TR |
									  SS_PART_FLAG_VERTEX_COLOR_BL |
									  SS_PART_FLAG_VERTEX_COLOR_BR,

	NUM_SS_PART_FLAGS
};

enum {
	SS_USER_DATA_FLAG_NUMBER		= 1 << 0,
	SS_USER_DATA_FLAG_RECT			= 1 << 1,
	SS_USER_DATA_FLAG_POINT			= 1 << 2,
	SS_USER_DATA_FLAG_STRING		= 1 << 3,

	NUM_SS_USER_DATA_FLAGS,

	SS_USER_DATA_FLAGS				= SS_USER_DATA_FLAG_NUMBER |
									  SS_USER_DATA_FLAG_RECT   |
									  SS_USER_DATA_FLAG_POINT  |
									  SS_USER_DATA_FLAG_STRING
};



/**
 * SSPlayer
 */

SSPlayer::SSPlayer(void)
	: m_ssDataHandle(0)
	, m_imageList(0)
	, m_frameSkipEnabled(true)
	, m_delegate(0)
	, m_playEndTarget(NULL)
	, m_playEndSelector(NULL)
	, m_batch(0)
	, m_ssPlayerScaleX( 1.0f )
	, m_ssPlayerScaleY( 1.0f )
	, m_ssPlayerFlipX( false )
	, m_ssPlayerFlipY( false )
	, m_integerPositionEnabled(false)
{
}

SSPlayer* SSPlayer::create()
{
	SSPlayer* player = new SSPlayer();
	if (player && player->init())
	{
		player->autorelease();
		return player;
	}
	CC_SAFE_DELETE(player);
	return NULL;
}
	
SSPlayer* SSPlayer::create(const SSData* ssData, SSImageList* imageList, int loop)
{
	SSPlayer* player = create();
	if (player)
	{
		player->setAnimation(ssData, imageList, loop);
	}
	return player;
}

SSPlayer::~SSPlayer()
{
	this->unscheduleUpdate();
	clearAnimation();
	releaseParts();
	
}

bool SSPlayer::init()
{
    if ( !CCSprite::init() )
    {
        return false;
    }

	return true;
}

void SSPlayer::allocParts(int numParts, bool useCustomShaderProgram)
{
	//BatchNodeの数はパーツ数ではなく、構造によって変わってくるので、setFrameメソッドで動的に行う
	//ここではインスタンスは生成せず、削除のみを行う
	//パーツ数が同じでも構造が違えばBatchNodeの数は変わってくるので、
	//以前のパーツ数に関わらず子要素を全て削除する
	this->removeAllChildrenWithCleanup(true);
	if (m_partStates.count() != numParts)
	{
		// 既存パーツ解放
		// release old parts.
		m_partStates.removeAllObjects();

		m_jointSprites.removeAllObjects();
		m_batchSprites.removeAllObjects();

		// パーツ数だけSPartStateを作成する
		// create SSPartState objects.
		for (int i = 0; i < numParts; i++)
		{
			SSPartState* state = new SSPartState();
			m_partStates.addObject(state);
		}
	}
	else
	{
		// SPartState初期化
		// initialize SPartState objects.
		for (int i = 0; i < numParts; i++)
		{
			SSPartState* partState = static_cast<SSPartState*>( m_partStates.objectAtIndex(i) );
			partState->init();
		}
	}
}

void SSPlayer::releaseParts()
{
	// パーツの子CCSpriteを全て削除
	// remove children CCSprite objects.
	this->removeAllChildrenWithCleanup(true);
	// パーツステートオブジェクトを全て削除
	// remove parts status objects.
	m_partStates.removeAllObjects();
	
	m_jointSprites.removeAllObjects();
	m_batchSprites.removeAllObjects();
}

bool SSPlayer::hasAnimation() const
{
	return m_ssDataHandle != 0;
}

void SSPlayer::clearAnimation()
{
	if (!hasAnimation()) return;

	CC_SAFE_DELETE(m_ssDataHandle);
	m_imageList->release();
	m_imageList = 0;
}

void SSPlayer::setAnimation(const SSData* ssData, SSImageList* imageList, int loop)
{
	CCAssert(ssData != NULL, "zero is ssData pointer");
	CCAssert(imageList != NULL, "zero is imageList pointer");

	this->unscheduleUpdate();//既存のSSPlayerに別のSSDataを読みこませようとすると落ちるので追加
	clearAnimation();

	SSDataHandle* dataHandle = new SSDataHandle(ssData);

	// パーツアロケート
	// allocate parts.
	bool useCustomShaderProgram = (dataHandle->getFlags() & SS_DATA_FLAG_USE_COLOR_BLEND) != 0;
	allocParts(dataHandle->getNumParts(), useCustomShaderProgram);

	// アニメーションパラメータ初期化
	// initialize animation parameters.
	m_ssDataHandle = dataHandle;
	imageList->retain();
	m_imageList = imageList;

	m_playingFrame = 0.0f;
	m_step = 1.0f;
	m_loop = loop;
	m_loopCount = 0;

	if (!m_batch)
	{
		setFrame(0);
		this->scheduleUpdate();
	}
}

const SSData* SSPlayer::getAnimation() const
{
	return m_ssDataHandle->getData();
}

void SSPlayer::update(float dt)
{
	if (!m_batch)
	{
		updateFrame(dt);
	}
}

void SSPlayer::updateFrame(float dt)
{
	if (!hasAnimation()) return;
	
	bool playEnd = false;
	if (m_loop == 0 || m_loopCount < m_loop)
	{
		// フレームを進める.
		// forward frame.
		const int numFrames = m_ssDataHandle->getNumFrames();

		float fdt = m_frameSkipEnabled ? dt : CCDirector::sharedDirector()->getAnimationInterval();
		float s = fdt / (1.0f / m_ssDataHandle->getFps());
		
		//if (!m_frameSkipEnabled) CCLOG("%f", s);
		
		float next = m_playingFrame + (s * m_step);

		int nextFrameNo = static_cast<int>(next);
		float nextFrameDecimal = next - static_cast<float>(nextFrameNo);
		int currentFrameNo = static_cast<int>(m_playingFrame);
		
		if (m_step >= 0)
		{
			// 順再生時.
			// normal plays.
			for (int c = nextFrameNo - currentFrameNo; c; c--)
			{
				int incFrameNo = currentFrameNo + 1;
				if (incFrameNo >= numFrames)
				{
					// アニメが一巡
					// turned animation.
					m_loopCount += 1;
					if (m_loop && m_loopCount >= m_loop)
					{
						// 再生終了.
						// play end.
						playEnd = true;
						break;
					}
					
					incFrameNo = 0;
				}
				currentFrameNo = incFrameNo;

				// このフレームのユーザーデータをチェック
				// check the user data of this frame.
				checkUserData(currentFrameNo);
			}
		}
		else
		{
			// 逆再生時.
			// reverse play.
			for (int c = currentFrameNo - nextFrameNo; c; c--)
			{
				int decFrameNo = currentFrameNo - 1;
				if (decFrameNo < 0)
				{
					// アニメが一巡
					// turned animation.
					m_loopCount += 1;
					if (m_loop && m_loopCount >= m_loop)
					{
						// 再生終了.
						// play end.
						playEnd = true;
						break;
					}
				
					decFrameNo = numFrames - 1;
				}
				currentFrameNo = decFrameNo;
				
				// このフレームのユーザーデータをチェック
				// check the user data of this frame.
				checkUserData(currentFrameNo);
			}
		}
		
		m_playingFrame = static_cast<float>(currentFrameNo) + nextFrameDecimal;
	}

	setFrame(getFrameNo());

	if (playEnd && m_playEndTarget)
	{
		(m_playEndTarget->*m_playEndSelector)(this);
	}
}

int SSPlayer::getFrameNo() const
{
	return static_cast<int>(m_playingFrame);
}

void SSPlayer::setFrameNo(int frameNo)
{
	m_playingFrame = frameNo;
}

float SSPlayer::getStep() const
{
	return m_step;
}

void SSPlayer::setStep(float step)
{
	m_step = step;
}

int SSPlayer::getLoop() const
{
	return m_loop;
}

void SSPlayer::setLoop(int loop)
{
	if (loop < 0) return;
	m_loop = loop;
}

int SSPlayer::getLoopCount() const
{
	return m_loopCount;
}

void SSPlayer::clearLoopCount()
{
	m_loopCount = 0;
}

void SSPlayer::setFrameSkipEnabled(bool enabled)
{
	m_frameSkipEnabled = enabled;
	m_playingFrame = (int)m_playingFrame;
}
	
bool SSPlayer::isFrameSkipEnabled() const
{
	return m_frameSkipEnabled;
}

void SSPlayer::setIntegerPositionEnabled(bool enabled)
{
	m_integerPositionEnabled = enabled;
}

bool SSPlayer::isIntegerPositionEnabled() const
{
	return m_integerPositionEnabled;
}

void SSPlayer::setDelegate(SSPlayerDelegate* delegate)
{
	m_delegate = delegate;
}

void SSPlayer::setPlayEndCallback(CCObject* target, SEL_PlayEndHandler selector)
{
	CC_SAFE_RELEASE(m_playEndTarget);
	CC_SAFE_RETAIN(target);
	m_playEndTarget = target;
	m_playEndSelector = selector;
}

bool SSPlayer::getPartState(SSPlayer::PartState& result, const char* name)
{
	if (hasAnimation())
	{
		int index = m_ssDataHandle->indexOfPart(name);
		if (index >= 0 && index < static_cast<int>(m_partStates.count()))
		{
			const SSPartState* partState = static_cast<SSPartState*>( m_partStates.objectAtIndex(index) );
			partState->copyParameters(result);
			return true;
		}
	}
	return false;
}

void SSPlayer::setFrame(int frameNo)
{
	setChildVisibleAll(false);

	// αブレンドでmix以外を使用、カラーブレンド、頂点変形が必要なものはバッチノードを使わず描画する
	bool useCustomSprite = (m_ssDataHandle->getFlags() & (SS_DATA_FLAG_USE_ALPHA_BLEND | SS_DATA_FLAG_USE_COLOR_BLEND | SS_DATA_FLAG_USE_VERTEX_OFFSET)) != 0;
	// カラーブレンドはカスタムシェーダーを使用する
	bool useCustomShaderProgram = (m_ssDataHandle->getFlags() & SS_DATA_FLAG_USE_COLOR_BLEND) != 0;
	// アフィン変換の有無
	bool useAffineTransformation = (m_ssDataHandle->getFlags() & SS_DATA_FLAG_USE_AFFINE_TRANS) != 0;


	const SSFrameData* frameData = &(m_ssDataHandle->getFrameData()[frameNo]);
	size_t numParts = static_cast<size_t>(frameData->numParts);
	SSDataReader r( static_cast<const ss_u16*>( m_ssDataHandle->getAddress(frameData->partFrameData)) );
	int nodeIndex = 0;//SSPlayerの子要素のCCSpriteBatchNodeのインデックス
	int spriteIndex = 0;//CCSpriteBatchNodeの子要素のスプライトのIndex


	CCNode* parentNode = NULL;
	CCSprite* jointNode = NULL;
	int jointNodeIndex = -1;
	bool jointToParentBatchNode = false;
	
	if (m_batch)
	{
		for (size_t i = 0; i < m_jointSprites.count(); i++)
		{
			CCNode* jointNode = (CCNode*)m_jointSprites.objectAtIndex(i);
			jointNode->removeAllChildrenWithCleanup(false);
		}
	}


	for (size_t i = 0; i < numParts; i++)
	{
		unsigned int flags = r.readU32();
		ss_u16 partNo = r.readU16();
		int sx = r.readS16();
		int sy = r.readS16();
		int sw = r.readS16();
		int sh = r.readS16();
		float dx = r.readFloat();
		float dy = r.readFloat();

		if (m_integerPositionEnabled)
		{
			dx = (int)dx;
			dy = (int)dy;
		}

		int ox = (flags & SS_PART_FLAG_ORIGIN_X) ? r.readS16() : sw / 2;
		int oy = (flags & SS_PART_FLAG_ORIGIN_Y) ? r.readS16() : sh / 2;

		float rotation = (flags & SS_PART_FLAG_ROTATION) ? -r.readFloat() : 0;
		float scaleX = (flags & SS_PART_FLAG_SCALE_X) ? r.readFloat() : 1.0f;
		float scaleY = (flags & SS_PART_FLAG_SCALE_Y) ? r.readFloat() : 1.0f;
		int opacity = (flags & SS_PART_FLAG_OPACITY) ? r.readU16() : 255;
	
		SSPartState* partState = static_cast<SSPartState*>( m_partStates.objectAtIndex(partNo) );
		partState->m_sprite = NULL;
		
		// パーツの基本情報を取得
		const SSPartData* partData = &m_ssDataHandle->getPartData()[partNo];
		size_t imageNo = partData->imageNo;
		CCTexture2D* tex = m_imageList->getTexture(imageNo);//このパーツの描画に使用するテクスチャ
		if(!tex){ continue; }
		SSPartType partType = static_cast<SSPartType>(partData->type);

		#if USE_CUSTOM_SPRITE
		SSSprite* sprite;
		#else
		CCSprite* sprite;
		#endif

		//SSPlayerの子要素数を取得。m_pChildrenは遅延生成されるためNULLチェック必須
		int childrenCount;
		if( m_pChildren ){ childrenCount = m_pChildren->count(); } else { childrenCount =  0; }
		
		bool setBlendEnabled = false;

		if (m_batch)
		{
			//bool useBatchNode = !useCustomSprite;
			bool useBatchNode =
				(flags & SS_PART_FLAGS_VERTEX_OFFSET) == 0 &&
				partData->alphaBlend == kSSPartAlphaBlendMix &&
				!useCustomShaderProgram;
			
			// 次のとき新たな親ノードを取得
			bool changeParentNode =
				(!parentNode) ||
				(useBatchNode != jointToParentBatchNode) ||
				(jointToParentBatchNode && jointNode->getTexture() != tex);
		
			if (changeParentNode)
			{
				m_batch->getNode(parentNode, useBatchNode, tex);
				
				jointNodeIndex++;
				if (jointNodeIndex >= m_jointSprites.count())
				{
					jointNode = CCSprite::createWithTexture(tex);
					jointNode->setTextureRect(CCRect(0, 0, 0, 0));
					m_jointSprites.addObject(jointNode);
				}
				else
				{
					jointNode = (CCSprite*)m_jointSprites.objectAtIndex(jointNodeIndex);
					jointNode->setTexture(tex);
				}
				
				jointNode->setPositionX(this->getPositionX());
				jointNode->setPositionY(this->getPositionY());
				jointNode->setScaleX(this->m_fScaleX);
				jointNode->setScaleY(this->m_fScaleY);
				jointNode->setRotation(this->getRotation());
				jointNode->setOpacity(this->getOpacity());
				
				parentNode->addChild(jointNode);
				jointToParentBatchNode = useBatchNode;
			}
		
		
			childrenCount = m_batchSprites.count();
			if (childrenCount <= i) {
#if USE_CUSTOM_SPRITE
				sprite =  SSSprite::create();
				sprite->changeShaderProgram(useCustomShaderProgram);
				sprite->setTexture(tex);
#else
				sprite = CCSprite::createWithTexture(tex);
#endif
				m_batchSprites.addObject(sprite);
			} else {
#if USE_CUSTOM_SPRITE
				sprite = static_cast<SSSprite*>( m_batchSprites.objectAtIndex(i) );
#else
				sprite = static_cast<CCSprite*>( m_batchSprites.objectAtIndex(i) );
#endif
				sprite->setTexture(tex);
			}
			
			jointNode->addChild(sprite);

			// ブレンド方法を設定する
			setBlendEnabled = true;

		}
		else if (!useCustomSprite)
		{
			//-------------------------------------------------------------
			//texと同じテクスチャを持っているバッチノードを探す
			//-------------------------------------------------------------
			CCSpriteBatchNode* node = NULL;//描画に使用するバッチノード
			if( nodeIndex < childrenCount ){
				//nodeIndexが指しているバッチノードが存在する
				//texと同じテクスチャを持っているか調べる
				node = static_cast<CCSpriteBatchNode*>( getChildren()->objectAtIndex(nodeIndex) );
				if( node->getTexture() != tex ){
					//描画したいテクスチャと同じテクスチャを持つバッチノードを順に検索する
					spriteIndex = 0;//バッチノードが変わるのでスプライトのインデックスを初期化
					do{
						++nodeIndex;
						if( nodeIndex == childrenCount ){
							//texを使用するバッチノードは見つからなかった
							node = NULL;
							break; 
						}
						//SSPlayerの子要素には全てCCSpriteBatchNodeがセットされているので、キャストして順に検索する
						node = static_cast<CCSpriteBatchNode*>( getChildren()->objectAtIndex(nodeIndex));
					}while(node->getTexture() != tex );
				}
			}
			
			//描画したいテクスチャを持つバッチノードが見つからなかった
			//子要素の最後尾に新しくバッチノードを作成し、追加する
			if( !node ){
				node = CCSpriteBatchNode::createWithTexture(tex);
				addChild(node);
			}
			
			//使用するバッチノードが決まったので表示状態にする
			node->setVisible(true);
			
			//このバッチノードの子要素の未使用スプライトを取得する
			//未使用スプライトが足りない場合は新規作成する
			CCArray* aNodeChildren = node->getChildren();
			int nodeChildrenCount;
			if( aNodeChildren ){ nodeChildrenCount = aNodeChildren->count(); } else { nodeChildrenCount =  0; }
			if( nodeChildrenCount == spriteIndex ){
				//スプライトの数が足りないのでバッチノードの子要素として新しく作成する
#if USE_CUSTOM_SPRITE
				sprite =  SSSprite::create();
				sprite->setTexture(node->getTexture());
				sprite->changeShaderProgram(useCustomShaderProgram);
#else
				sprite = CCSprite::createWithTexture(tex);
#endif
				node->addChild(sprite);
			} else {
				//バッチノードの子要素のスプライトの数は足りているので、未使用のスプライトを描画に使用する
#if USE_CUSTOM_SPRITE
				sprite = static_cast<SSSprite*>( node->getChildren()->objectAtIndex( spriteIndex ) );
#else
				sprite = static_cast<CCSprite*>( node->getChildren()->objectAtIndex( spriteIndex ) );
#endif
			}
			//このノードの子要素のspiretIndex番目のスプライトは使用済みなので、インデックスをインクリメントする
			++spriteIndex;
		}
		else {
			// バッチノードを使わず描画
			if (childrenCount <= i) {
#if USE_CUSTOM_SPRITE
				sprite =  SSSprite::create();
				sprite->changeShaderProgram(useCustomShaderProgram);
				sprite->setTexture(tex);
#else
				sprite = CCSprite::createWithTexture(tex);
#endif
				addChild(sprite);
			} else {
#if USE_CUSTOM_SPRITE
				sprite = static_cast<SSSprite*>( m_pChildren->objectAtIndex(i) );
#else
				sprite = static_cast<CCSprite*>( m_pChildren->objectAtIndex(i) );
#endif
				sprite->setTexture(tex);
			}

			// ブレンド方法を設定する
			setBlendEnabled = true;
		}

		
		if (setBlendEnabled)
		{
			//
			// ブレンド方法を設定
			// 標準状態でMIXブレンド相当になります
			// BlendFuncの値を変更することでブレンド方法を切り替えます
			//
			ccBlendFunc blendFunc = sprite->getBlendFunc();
			if (!tex->hasPremultipliedAlpha())
			{
				blendFunc.src = GL_SRC_ALPHA;
				blendFunc.dst = GL_ONE_MINUS_SRC_ALPHA;
			}
			else
			{
				blendFunc.src = CC_BLEND_SRC;
				blendFunc.dst = CC_BLEND_DST;
			}

			// カスタムシェーダを使用する場合
			if (useCustomShaderProgram) {
				blendFunc.src = GL_SRC_ALPHA;
			}
			// 加算ブレンド
			if (partData->alphaBlend == kSSPartAlphaBlendAddition) {
				blendFunc.dst = GL_ONE;
			}
			sprite->setBlendFunc(blendFunc);
		}

#if ADJUST_UV_BY_CONTENT_SCALE_FACTOR
		CCRect orgRect(sx, sy, sw, sh);
		float sf = CC_CONTENT_SCALE_FACTOR();
		float ssx = (float)sx / sf;
		float ssy = (float)sy / sf;
		float ssw = (float)sw / sf;
		float ssh = (float)sh / sf;
		sprite->setTextureRect(CCRect(ssx, ssy, ssw, ssh), false, orgRect.size);
#else
		sprite->setTextureRect(CCRect(sx, sy, sw, sh));
#endif

		sprite->setOpacity( opacity );
		
		float ax = (float)ox / (float)sw;
		float ay = (float)oy / (float)sh;
		sprite->setAnchorPoint(ccp(ax, ay));

		sprite->setFlipX((flags & SS_PART_FLAG_FLIP_H) != 0);
		sprite->setFlipY((flags & SS_PART_FLAG_FLIP_V) != 0);
		sprite->setRotation(rotation);
        sprite->setScaleX(scaleX);
        sprite->setScaleY(scaleY);
		sprite->setPosition(ccp(dx, -dy));

		
		ccV3F_C4B_T2F_Quad tempQuad;

		#if USE_CUSTOM_SPRITE
		ccV3F_C4B_T2F_Quad& vquad = sprite->getAttributeRef();
		ccV3F_C4B_T2F_Quad& cquad = sprite->isCustomShaderProgramEnabled() ? sprite->getAttributeRef() : tempQuad;
		#else
		ccV3F_C4B_T2F_Quad& vquad = tempQuad;
		ccV3F_C4B_T2F_Quad& cquad = tempQuad;
		#endif

		// vertex deformation
		if (flags & SS_PART_FLAG_VERTEX_OFFSET_TL)
		{
			vquad.tl.vertices.x += r.readS16();
			vquad.tl.vertices.y -= r.readS16();
		}
		if (flags & SS_PART_FLAG_VERTEX_OFFSET_TR)
		{
			vquad.tr.vertices.x += r.readS16();
			vquad.tr.vertices.y -= r.readS16();
		}
		if (flags & SS_PART_FLAG_VERTEX_OFFSET_BL)
		{
			vquad.bl.vertices.x += r.readS16();
			vquad.bl.vertices.y -= r.readS16();
		}
		if (flags & SS_PART_FLAG_VERTEX_OFFSET_BR)
		{
			vquad.br.vertices.x += r.readS16();
			vquad.br.vertices.y -= r.readS16();
		}


		// color blend
		ccColor4B color4 = { 0xff, 0xff, 0xff, 0 };
		cquad.tl.colors =
		cquad.tr.colors =
		cquad.bl.colors =
		cquad.br.colors = color4;

		if (flags & SS_PART_FLAGS_COLOR_BLEND)
		{
			int colorBlendFuncNo = r.readU16();
			#if USE_CUSTOM_SPRITE
			sprite->setColorBlendFunc(colorBlendFuncNo);
			#endif
		
			if (flags & SS_PART_FLAG_COLOR)
			{
				r.readColor(color4);
				cquad.tl.colors =
				cquad.tr.colors =
				cquad.bl.colors =
				cquad.br.colors = color4;
			}
			if (flags & SS_PART_FLAG_VERTEX_COLOR_TL)
			{
				r.readColor(color4);
				cquad.tl.colors = color4;
			}
			if (flags & SS_PART_FLAG_VERTEX_COLOR_TR)
			{
				r.readColor(color4);
				cquad.tr.colors = color4;
			}
			if (flags & SS_PART_FLAG_VERTEX_COLOR_BL)
			{
				r.readColor(color4);
				cquad.bl.colors = color4;
			}
			if (flags & SS_PART_FLAG_VERTEX_COLOR_BR)
			{
				r.readColor(color4);
				cquad.br.colors = color4;
			}
		}

		// この時点の座標、スケール値などを記録しておく
		partState->m_x = sprite->getPositionX();
		partState->m_y = sprite->getPositionY();
		partState->m_scaleX = sprite->getScaleX();
		partState->m_scaleY = sprite->getScaleY();
		partState->m_rotation = sprite->getRotation();
		partState->m_sprite = sprite;

		// Normalパーツのみ実際に表示する
		bool visibled = (partType == kSSPartTypeNormal) && !(flags & SS_PART_FLAG_INVISIBLE);
		sprite->setVisible(visibled);
	}

#if (COCOS2D_VERSION >= 0x00020100)
	if (useAffineTransformation)
	{
		//親のアフィン変換を適用するコード(SS5準拠）
		//ルートパーツ以外を処理
		size_t partsCount = m_ssDataHandle->getNumParts();
		for (size_t partNo = 1; partNo < partsCount; partNo++)
		{
			CCAffineTransform trans = CCAffineTransformMakeIdentity();

			//親子を調べる
			const SSPartData* partData = &m_ssDataHandle->getPartData()[partNo];
			SSPartState* paernt_partState = static_cast<SSPartState*>( m_partStates.objectAtIndex(partData->parentId) );
			trans = paernt_partState->m_trans;

			trans = CCAffineTransformTranslate( trans , paernt_partState->m_x , paernt_partState->m_y );
			trans = CCAffineTransformRotate( trans , CC_DEGREES_TO_RADIANS(-paernt_partState->m_rotation) );// Rad?
			trans = CCAffineTransformScale( trans , paernt_partState->m_scaleX,paernt_partState->m_scaleY );

			SSPartState* partState = static_cast<SSPartState*>( m_partStates.objectAtIndex(partNo) );
//			CCLOG("addr: %08lx, %08lx, %f, %f, %f, %f, %f", partState, partState->m_sprite, partState->m_x, partState->m_y, partState->m_scaleX, partState->m_scaleY, partState->m_rotation);
//			if (!partState->m_sprite) continue;

			partState->m_trans = trans;
			if (partState->m_sprite)
			{
				partState->m_sprite->setAdditionalTransform( trans );
			}
		}
	}
#endif
}

void SSPlayer::setFlipX(bool bFlipX)
{
	m_ssPlayerFlipX = bFlipX;
	setScaleX(m_ssPlayerScaleX); 
}

void SSPlayer::setFlipY(bool bFlipY)
{
	m_ssPlayerFlipY = bFlipY;
	setScaleX(m_ssPlayerScaleY); 
}

void SSPlayer::setScaleX(float fScaleX)
{
	m_ssPlayerScaleX = fScaleX;	
	float temp = m_ssPlayerFlipX == true ? -m_ssPlayerScaleX : m_ssPlayerScaleX;
	cocos2d::CCSprite::setScaleX(temp);
}

void SSPlayer::setScaleY(float fScaleY)
{
	m_ssPlayerScaleY = fScaleY;	
	float temp = m_ssPlayerFlipY == true ? -m_ssPlayerScaleY : m_ssPlayerScaleY;
	cocos2d::CCSprite::setScaleY(temp);
}

float SSPlayer::getScaleX()
{
	return m_ssPlayerScaleX;
}

float SSPlayer::getScaleY()
{
	return m_ssPlayerScaleY;
}

float SSPlayer::getScale()
{
    CCAssert( m_ssPlayerScaleX == m_ssPlayerScaleY, "SSPlayer#scale. ScaleX != ScaleY. Don't know which one to return");
	return m_ssPlayerScaleX;
}

void SSPlayer::setChildVisibleAll(bool visible)
{
	//SSPlayerの子要素のバッチノードと、バッチノードの子要素のCCSpriteのVisibleを全てセットする
	CCArray* a = getChildren();
	int pn = getChildrenCount();
	for( int i = 0; i < pn; ++i){
		CCNode* nodep = static_cast<CCNode*>( a->objectAtIndex(i));
		nodep->setVisible(visible);
		int pc = nodep->getChildrenCount();
		for( int j = 0; j < pc; ++j){
			static_cast<CCNode*>( nodep->getChildren()->objectAtIndex(j))->setVisible(visible);
		}
	}
}

void SSPlayer::checkUserData(int frameNo)
{
	if (!m_delegate) return;

	const SSFrameData* frameData = &(m_ssDataHandle->getFrameData()[frameNo]);
	size_t numUserData = static_cast<size_t>(frameData->numUserData);
	SSDataReader r( static_cast<const ss_u16*>( m_ssDataHandle->getAddress(frameData->userData)) );

	for (size_t i = 0; i < numUserData; i++)
	{
		int flags = r.readU16();
		int partId = r.readU16();

		m_userData.flags = 0;

		if (flags & SS_USER_DATA_FLAG_NUMBER)
		{
			m_userData.flags |= SSUserData::FLAG_NUMBER;
			m_userData.number = r.readS32();
		}
		else
		{
			m_userData.number = 0;
		}
		
		if (flags & SS_USER_DATA_FLAG_RECT)
		{
			m_userData.flags |= SSUserData::FLAG_RECT;
			m_userData.rect[0] = r.readS32();
			m_userData.rect[1] = r.readS32();
			m_userData.rect[2] = r.readS32();
			m_userData.rect[3] = r.readS32();
		}
		else
		{
			m_userData.rect[0] =
			m_userData.rect[1] =
			m_userData.rect[2] =
			m_userData.rect[3] = 0;
		}
		
		if (flags & SS_USER_DATA_FLAG_POINT)
		{
			m_userData.flags |= SSUserData::FLAG_POINT;
			m_userData.point[0] = r.readS32();
			m_userData.point[1] = r.readS32();
		}
		else
		{
			m_userData.point[0] =
			m_userData.point[1] = 0;
		}
		
		if (flags & SS_USER_DATA_FLAG_STRING)
		{
			m_userData.flags |= SSUserData::FLAG_STRING;
			int length;
			m_userData.str = r.getString(&length);
		}
		else
		{
			m_userData.str = 0;
		}
		
		const char* partName = m_ssDataHandle->getPartName(partId);
		m_delegate->onUserData(this, &m_userData, frameNo, partName);
	}
}

void SSPlayer::registerBatch(SSPlayerBatch *batch)
{
	this->unscheduleUpdate();
	m_batch = batch;
}

void SSPlayer::unregisterBatch(SSPlayerBatch *batch)
{
	m_batch = 0;
	this->scheduleUpdate();
}



/**
 * SSPlayerDelegate
 */

SSPlayerDelegate::~SSPlayerDelegate()
{}

void SSPlayerDelegate::onUserData(SSPlayer* player, const SSUserData* data, int frameNo, const char* partName)
{}



/**
 * SSPlayerBatch
 *
 * SSPlayerをSSPlayerBatch配下に配置（addChild）することにより、
 * できる限りバッチノードを使った描画を行うようになります。
 * ※SSPlayerBatchはSSPlayerのみ登録可能です。
 *
 * 以下の条件を満たしているとき、バッチノードを使った描画が行われます。（条件に合わないパーツは単独で描画されます）
 * ・同一のテクスチャを使用している
 * ・次の効果を使用していない：頂点変形、カラーブレンド、αブレンドでMIX以外
 */

SSPlayerBatch::SSPlayerBatch()
	: m_players(NULL)
	, m_bundles(NULL)
	, m_defaultCapacity(kDefaultSpriteBatchCapacity)
{
}

SSPlayerBatch::~SSPlayerBatch()
{
	this->unscheduleUpdate();
	CC_SAFE_RELEASE_NULL(m_bundles);
	CC_SAFE_RELEASE_NULL(m_players);
}

SSPlayerBatch* SSPlayerBatch::create()
{
	SSPlayerBatch* batch = new SSPlayerBatch();
	if (batch && batch->init())
	{
		batch->autorelease();
		return batch;
	}
	CC_SAFE_DELETE(batch);
	return NULL;
}

bool SSPlayerBatch::init()
{
    if (!CCNode::init())
	{
		return false;
	}
	
	CC_SAFE_RELEASE_NULL(m_bundles);
	CC_SAFE_RELEASE_NULL(m_players);
	
	CCNode* players = CCNode::create();
	if (!players)
	{
		return false;
	}
	
	CCNode* bundles = CCNode::create();
	if (!bundles)
	{
		delete players;
		return false;
	}
	
	players->retain();
	bundles->retain();
	
	m_players = players;
	players->setVisible(false);
	CCNode::addChild(players, 0, 0);
	m_bundles = bundles;
	CCNode::addChild(bundles, 0, 0);
	
	this->scheduleUpdate();
	return true;
}

void SSPlayerBatch::setDefaultSpriteBatchCapacity(unsigned int capacity)
{
	m_defaultCapacity = capacity;
}

void SSPlayerBatch::addChild(CCNode* child, int zOrder, int tag)
{
    CCAssert(child != NULL, "child should not be null");
    CCAssert(dynamic_cast<SSPlayer*>(child) != NULL, "SSPlayerBatch only supports SSPlayer as children");
    SSPlayer* player = (SSPlayer*)(child);

	m_players->addChild(child, zOrder, tag);
	player->registerBatch(this);
}

void SSPlayerBatch::addChild(CCNode* child, int zOrder)
{
	CCNode::addChild(child, zOrder);
}

void SSPlayerBatch::addChild(CCNode* child)
{
	CCNode::addChild(child);
}

void SSPlayerBatch::removeChild(CCNode* child)
{
    CCAssert(child != NULL, "child should not be null");
    SSPlayer* player = (SSPlayer*)(child);

	player->unregisterBatch(this);
	m_players->removeChild(player);
}

enum SSPlayerBatchTag
{
	SSPLAYERBATCHTAG_NODE = 1,
	SSPLAYERBATCHTAG_BATCH_NODE
};

void SSPlayerBatch::update(float dt)
{
	m_currentNodeIndex = -1;
	m_currentNode = NULL;
	m_currentBatchNode = NULL;
	m_isBatchNodeCurrent = false;
	m_currentTexture = NULL;
	
	CCObject* child;

	if (m_bundles->getChildren())
	{
		CCARRAY_FOREACH(m_bundles->getChildren(), child)
		{
			CCNode* bundleNode = (CCNode*)child;
			if (bundleNode && bundleNode->isVisible())
			{
				bundleNode->setVisible(false);
				CCNode* node = (CCNode*)bundleNode->getChildByTag(SSPLAYERBATCHTAG_NODE);
				CCSpriteBatchNode* batchNode = (CCSpriteBatchNode*)bundleNode->getChildByTag(SSPLAYERBATCHTAG_BATCH_NODE);
				node->removeAllChildrenWithCleanup(false);
				batchNode->removeAllChildrenWithCleanup(false);
			}
		}
	}

	if (m_players->getChildren())
	{
		CCARRAY_FOREACH(m_players->getChildren(), child)
		{
			SSPlayer* player = (SSPlayer*)child;
			if (player)
			{
				player->updateFrame(dt);
			}
		}
	}
}

void SSPlayerBatch::getNode(cocos2d::CCNode*& node, bool batchNodeRequired, cocos2d::CCTexture2D* tex)
{
	bool nextNode =
		(m_currentNode == NULL) ||
		(batchNodeRequired != m_isBatchNodeCurrent) ||
		(m_isBatchNodeCurrent && tex != m_currentTexture);
		
	if (nextNode)
	{
		m_currentNodeIndex++;
		if (!m_bundles->getChildren() || m_currentNodeIndex >= m_bundles->getChildren()->count())
		{
			// 新しくノードを生成する
			m_currentNode = CCNode::create();
			m_currentBatchNode = CCSpriteBatchNode::createWithTexture(tex, m_defaultCapacity);

			CCNode* bundleNode = CCNode::create();
			bundleNode->addChild(m_currentNode, 0, SSPLAYERBATCHTAG_NODE);
			bundleNode->addChild(m_currentBatchNode, 0, SSPLAYERBATCHTAG_BATCH_NODE);
			m_bundles->addChild(bundleNode);
		}
		else
		{
			// 既存のノードを流用
			CCNode* bundleNode = (CCNode*)m_bundles->getChildren()->objectAtIndex(m_currentNodeIndex);
			m_currentNode = (CCNode*)bundleNode->getChildByTag(SSPLAYERBATCHTAG_NODE);
			m_currentBatchNode = (CCSpriteBatchNode*)bundleNode->getChildByTag(SSPLAYERBATCHTAG_BATCH_NODE);
			m_currentBatchNode->setTexture(tex);
			bundleNode->setVisible(true);
		}
		m_isBatchNodeCurrent = batchNodeRequired;
		m_currentTexture = tex;
	}

	node = m_isBatchNodeCurrent ? m_currentBatchNode : m_currentNode;
}





#if USE_CUSTOM_SPRITE

/**
 * SSSprite
 */

unsigned int SSSprite::ssSelectorLocation = 0;
unsigned int SSSprite::ssAlphaLocation = 0;

static const GLchar * ssPositionTextureColor_frag =
#include "ssShader_frag.h"

SSSprite::SSSprite()
	: _defaultShaderProgram(NULL)
	, _useCustomShaderProgram(false)
	, _opacity(1.0f)
	, _colorBlendFuncNo(0)
{}

SSSprite::~SSSprite()
{}

CCGLProgram* SSSprite::getCustomShaderProgram()
{
	static CCGLProgram* p = NULL;
	static bool constructFailed = false;
	if (!p && !constructFailed)
	{
		p = new CCGLProgram();
		p->initWithVertexShaderByteArray(
			ccPositionTextureColor_vert,
			ssPositionTextureColor_frag);
		p->addAttribute(kCCAttributeNamePosition, kCCVertexAttrib_Position);
		p->addAttribute(kCCAttributeNameColor, kCCVertexAttrib_Color);
		p->addAttribute(kCCAttributeNameTexCoord, kCCVertexAttrib_TexCoords);

		if (!p->link())
		{
			constructFailed = true;
			return NULL;
		}
		
		p->updateUniforms();
		
		ssSelectorLocation = glGetUniformLocation(p->getProgram(), "u_selector");
		ssAlphaLocation = glGetUniformLocation(p->getProgram(), "u_alpha");
		if (ssSelectorLocation == GL_INVALID_VALUE
		 || ssAlphaLocation == GL_INVALID_VALUE)
		{
			delete p;
			p = NULL;
			constructFailed = true;
			return NULL;
		}

		glUniform1i(ssSelectorLocation, 0);
		glUniform1f(ssAlphaLocation, 1.0f);
	}
	return p;
}

SSSprite* SSSprite::create()
{
	SSSprite *pSprite = new SSSprite();
	if (pSprite && pSprite->init())
	{
		pSprite->_defaultShaderProgram = pSprite->getShaderProgram();
		pSprite->autorelease();
		return pSprite;
	}
	CC_SAFE_DELETE(pSprite);
	return NULL;
}

void SSSprite::changeShaderProgram(bool useCustomShaderProgram)
{
	if (useCustomShaderProgram != _useCustomShaderProgram)
	{
		if (useCustomShaderProgram)
		{
			CCGLProgram *shaderProgram = getCustomShaderProgram();
			if (shaderProgram == NULL)
			{
				// Not use custom shader.
				shaderProgram = _defaultShaderProgram;
				useCustomShaderProgram = false;
			}
			this->setShaderProgram(shaderProgram);
			_useCustomShaderProgram = useCustomShaderProgram;
		}
		else
		{
			this->setShaderProgram(_defaultShaderProgram);
			_useCustomShaderProgram = false;
		}
	}
}

bool SSSprite::isCustomShaderProgramEnabled() const
{
	return _useCustomShaderProgram;
}

void SSSprite::setColorBlendFunc(int colorBlendFuncNo)
{
	_colorBlendFuncNo = colorBlendFuncNo;
}

ccV3F_C4B_T2F_Quad& SSSprite::getAttributeRef()
{
	return m_sQuad;
}

void SSSprite::setOpacity(GLubyte opacity)
{
	CCSprite::setOpacity(opacity);
	_opacity = static_cast<float>(opacity) / 255.0f;
}

#if ADJUST_UV_BY_CONTENT_SCALE_FACTOR
void SSSprite::setVertexRect(const CCRect& rect)
{
	float sf = CC_CONTENT_SCALE_FACTOR();
	m_obRect = CCRect(rect.origin.x * sf, rect.origin.y * sf, rect.size.width * sf, rect.size.height * sf);
}
#endif


void SSSprite::draw(void)
{
    CC_PROFILER_START_CATEGORY(kCCProfilerCategorySprite, "SSSprite - draw");
	
	
	if (!_useCustomShaderProgram)
	{
		CCSprite::draw();
		return;
	}
	

    CCAssert(!m_pobBatchNode, "If CCSprite is being rendered by CCSpriteBatchNode, CCSprite#draw SHOULD NOT be called");

    CC_NODE_DRAW_SETUP();

    ccGLBlendFunc( m_sBlendFunc.src, m_sBlendFunc.dst );

    if (m_pobTexture != NULL)
    {
        ccGLBindTexture2D( m_pobTexture->getName() );
    }
    else
    {
        ccGLBindTexture2D(0);
    }
    
	glUniform1i(ssSelectorLocation, _colorBlendFuncNo);
	glUniform1f(ssAlphaLocation, _opacity);


    //
    // Attributes
    //

    ccGLEnableVertexAttribs( kCCVertexAttribFlag_PosColorTex );

#define kQuadSize sizeof(m_sQuad.bl)
    long offset = (long)&m_sQuad;

    // vertex
    int diff = offsetof( ccV3F_C4B_T2F, vertices);
    glVertexAttribPointer(kCCVertexAttrib_Position, 3, GL_FLOAT, GL_FALSE, kQuadSize, (void*) (offset + diff));

    // texCoods
    diff = offsetof( ccV3F_C4B_T2F, texCoords);
    glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, kQuadSize, (void*)(offset + diff));

    // color
    diff = offsetof( ccV3F_C4B_T2F, colors);
    glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, kQuadSize, (void*)(offset + diff));


    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    CHECK_GL_ERROR_DEBUG();


#if CC_SPRITE_DEBUG_DRAW == 1
    // draw bounding box
    CCPoint vertices[4]={
        ccp(m_sQuad.tl.vertices.x,m_sQuad.tl.vertices.y),
        ccp(m_sQuad.bl.vertices.x,m_sQuad.bl.vertices.y),
        ccp(m_sQuad.br.vertices.x,m_sQuad.br.vertices.y),
        ccp(m_sQuad.tr.vertices.x,m_sQuad.tr.vertices.y),
    };
    ccDrawPoly(vertices, 4, true);
#elif CC_SPRITE_DEBUG_DRAW == 2
    // draw texture box
    CCSize s = this->getTextureRect().size;
    CCPoint offsetPix = this->getOffsetPosition();
    CCPoint vertices[4] = {
        ccp(offsetPix.x,offsetPix.y), ccp(offsetPix.x+s.width,offsetPix.y),
        ccp(offsetPix.x+s.width,offsetPix.y+s.height), ccp(offsetPix.x,offsetPix.y+s.height)
    };
    ccDrawPoly(vertices, 4, true);
#endif // CC_SPRITE_DEBUG_DRAW

    CC_INCREMENT_GL_DRAWS(1);

    CC_PROFILER_STOP_CATEGORY(kCCProfilerCategorySprite, "CCSprite - draw");
}

#endif	// if USE_CUSTOM_SPRITE


