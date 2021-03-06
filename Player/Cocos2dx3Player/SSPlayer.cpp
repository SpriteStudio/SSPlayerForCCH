﻿
#include "SSPlayer.h"
#include "SSPlayerData.h"
#include <cstring>
#include <string>

using namespace cocos2d;


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


/**
 * definition
 */

static const ss_u32 SSDATA_ID_0 = 0xffffffff;
static const ss_u32 SSDATA_ID_1 = 0x53534241;



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
	std::string fullpath = FileUtils::sharedFileUtils()->fullPathForFilename(path.c_str());
#else
	std::string fullpath = CCFileUtils::sharedFileUtils()->fullPathFromRelativePath(path.c_str());
#endif
	unsigned long nSize = 0;
	unsigned char* data = FileUtils::sharedFileUtils()->getFileData(fullpath.c_str(), "rb", &nSize);
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
	}
	
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
	TextureCache* texCache = TextureCache::sharedTextureCache();
	for (size_t i = 0, count = m_imageList.count(); i < count; i++)
	{
		Texture2D* tex = static_cast<Texture2D*>( m_imageList.objectAtIndex(i) );
		texCache->removeTexture(tex);
	}

	m_imageList.removeAllObjects();
}

Texture2D* SSImageList::getTexture(size_t index)
{
	if (index >= m_imageList.count()) return NULL;

	Texture2D* tex = static_cast<Texture2D*>( m_imageList.objectAtIndex(index) );
	return tex;
}

void SSImageList::addTexture(const char* imageName, const char* imageDir)
{
	std::string path;
	if (imageDir) path.append(imageDir);
	path.append(imageName);
	
	TextureCache* texCache = TextureCache::sharedTextureCache();
	Texture2D* tex = texCache->addImage(path.c_str());
	if (tex == NULL)
	{
		CCLOG("image load failed: %s", path.c_str());
		CC_ASSERT(0);
	}
	m_imageList.addObject(tex);
}



/**
 * SSPartState
 */

class SSPartState : public cocos2d::Object
{
public:
	SSPartState();
	virtual ~SSPartState();

	float getPositionX() const;
	float getPositionY() const;

private:
	friend class SSPlayer;

	float	m_x;
	float	m_y;
};

SSPartState::SSPartState()
	: m_x(0), m_y(0)
{
	this->autorelease();
}

SSPartState::~SSPartState()
{
}

float SSPartState::getPositionX() const
{
	return m_x;
}

float SSPartState::getPositionY() const
{
	return m_y;
}




#if USE_CUSTOM_SPRITE

/**
 * SSSprite
 */

class SSSprite : public Sprite
{
private:
	static unsigned int ssSelectorLocation;
	static unsigned int	ssAlphaLocation;

	static GLProgram* getCustomShaderProgram();

private:
	GLProgram*	_defaultShaderProgram;
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

	NUM_SS_DATA_FLAGS
};

enum {
	SS_PART_FLAG_FLIP_H				= 1 << 0,
	SS_PART_FLAG_FLIP_V				= 1 << 1,
	
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
	
SSPlayer* SSPlayer::create(const SSData* ssData, SSImageList* imageList)
{
	SSPlayer* player = create();
	if (player)
	{
		player->setAnimation(ssData, imageList);
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
	if (m_partStates.count() != numParts)
	{
		// 既存パーツ解放
		// release old parts.
		releaseParts();

		// パーツ数だけCCSpriteとSSPartStateを作成する
		// create CCSprite objects and SSPartState objects.
		for (int i = 0; i < numParts; i++)
		{
			#if USE_CUSTOM_SPRITE
			SSSprite* pSprite = SSSprite::create();
			pSprite->changeShaderProgram(useCustomShaderProgram);
			#else
			Sprite* pSprite = Sprite::create();
			#endif
			
			this->addChild(pSprite);

			SSPartState* state = new SSPartState();
			m_partStates.addObject(state);
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

void SSPlayer::setAnimation(const SSData* ssData, SSImageList* imageList)
{
	CCAssert(ssData != NULL, "zero is ssData pointer");
	CCAssert(imageList != NULL, "zero is imageList pointer");

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
	m_loop = 0;
	m_loopCount = 0;

	setFrame(0);

	this->scheduleUpdate();
}

void SSPlayer::update(float dt)
{
//	CCLOG("%f", dt);

	if (!hasAnimation()) return;

	if (m_loop == 0 || m_loopCount < m_loop)
	{
		// フレームを進める.
		// forward frame.
		const int numFrames = m_ssDataHandle->getNumFrames();

		float fdt = m_frameSkipEnabled ? dt : CCDirector::sharedDirector()->getAnimationInterval();
		float s = fdt / (1.0f / m_ssDataHandle->getFps());
		
		if (!m_frameSkipEnabled) CCLOG("%f", s);
		
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

void SSPlayer::setDelegate(SSPlayerDelegate* delegate)
{
	m_delegate = delegate;
}

bool SSPlayer::getPartState(SSPlayer::PartState& result, const char* name)
{
	if (hasAnimation())
	{
		int index = m_ssDataHandle->indexOfPart(name);
		if (index >= 0 && index < static_cast<int>(m_partStates.count()))
		{
			const SSPartState* partState = static_cast<SSPartState*>( m_partStates.objectAtIndex(index) );
			result.x = partState->getPositionX();
			result.y = partState->getPositionY();
			return true;
		}
	}
	return false;
}

void SSPlayer::setFrame(int frameNo)
{
	setChildVisibleAll(false);

	const SSFrameData* frameData = &(m_ssDataHandle->getFrameData()[frameNo]);
	size_t numParts = static_cast<size_t>(frameData->numParts);
	SSDataReader r( static_cast<const ss_u16*>( m_ssDataHandle->getAddress(frameData->partFrameData)) );
	for (size_t i = 0; i < numParts; i++)
	{
		unsigned int flags = r.readU32();
		ss_u16 partNo = r.readU16();
		int sx = r.readS16();
		int sy = r.readS16();
		int sw = r.readS16();
		int sh = r.readS16();
		int dx = r.readS16();
		int dy = r.readS16();

		int ox = (flags & SS_PART_FLAG_ORIGIN_X) ? r.readS16() : sw / 2;
		int oy = (flags & SS_PART_FLAG_ORIGIN_Y) ? r.readS16() : sh / 2;

		float rotation = (flags & SS_PART_FLAG_ROTATION) ? -r.readFloat() : 0;
		float scaleX = (flags & SS_PART_FLAG_SCALE_X) ? r.readFloat() : 1.0f;
		float scaleY = (flags & SS_PART_FLAG_SCALE_Y) ? r.readFloat() : 1.0f;
		int opacity = (flags & SS_PART_FLAG_OPACITY) ? r.readU16() : 255;
	
		#if USE_CUSTOM_SPRITE
		SSSprite* sprite = static_cast<SSSprite*>( _children->objectAtIndex(i) );
		#else
		Sprite* sprite = static_cast<Sprite*>( _children->objectAtIndex(i) );
		#endif

		SSPartState* partState = static_cast<SSPartState*>( m_partStates.objectAtIndex(partNo) );
		size_t imageNo = m_ssDataHandle->getPartData()[partNo].imageNo;


		Texture2D* tex = m_imageList->getTexture(imageNo);
        if (tex == NULL) continue;
		sprite->setTexture(tex);
		sprite->setTextureRect(Rect(sx, sy, sw, sh));

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

		sprite->setVisible(true);

		partState->m_x = sprite->getPositionX();
		partState->m_y = sprite->getPositionY();
	}
}

void SSPlayer::setChildVisibleAll(bool visible)
{
    Object* child;
    CCARRAY_FOREACH(_children, child)
	{
		Node* node = static_cast<Node*>(child);
		node->setVisible(visible);
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



/**
 * SSPlayerDelegate
 */

SSPlayerDelegate::~SSPlayerDelegate()
{}

void SSPlayerDelegate::onUserData(SSPlayer* player, const SSUserData* data, int frameNo, const char* partName)
{}



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

GLProgram* SSSprite::getCustomShaderProgram()
{
	static GLProgram* p = NULL;
	static bool constructFailed = false;
	if (!p && !constructFailed)
	{
		p = new GLProgram();
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
	return _quad;
}

void SSSprite::setOpacity(GLubyte opacity)
{
	CCSprite::setOpacity(opacity);
	_opacity = static_cast<float>(opacity) / 255.0f;
}

void SSSprite::draw(void)
{
    CC_PROFILER_START_CATEGORY(kCCProfilerCategorySprite, "SSSprite - draw");
	
	
	if (!_useCustomShaderProgram)
	{
		CCSprite::draw();
		return;
	}
	

    CCASSERT(!_batchNode, "If Sprite is being rendered by SpriteBatchNode, Sprite#draw SHOULD NOT be called");

    CC_NODE_DRAW_SETUP();

    //ccGLBlendFunc( m_sBlendFunc.src, m_sBlendFunc.dst );
    GL::blendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    if (_texture != NULL)
    {
        GL::bindTexture2D( _texture->getName() );
    }
    else
    {
        GL::bindTexture2D(0);
    }
    
	glUniform1i(ssSelectorLocation, _colorBlendFuncNo);
	glUniform1f(ssAlphaLocation, _opacity);


    //
    // Attributes
    //

    ccGLEnableVertexAttribs( kCCVertexAttribFlag_PosColorTex );

#define kQuadSize sizeof(_quad.bl)
    long offset = (long)&_quad;

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


