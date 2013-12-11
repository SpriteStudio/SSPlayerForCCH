#ifndef __SS_PLAYER_H__
#define __SS_PLAYER_H__

#include "cocos2d.h"

#include "SSPlayerData.h"

class SSPlayerDelegate;
class SSPlayerBatch;


/**
 * SSImageList
 */

class SSImageList : public cocos2d::CCObject
{
public:
	/** SSImageListを生成します.
	 *  Create a SSImageList object.
	 */
	static SSImageList* create();

	/** SSImageListを生成し、アニメーションデータから初期化します.
	 *  Create a SSImageList object, and initialize from animation data.
	 */
	static SSImageList* create(const SSData* ssData, const char* imageDir = NULL);

	/** SSImageListを生成し、ファイル名リストから初期化します.
	 *  Create a SSImageList object, and initialize from filename list.
	 */
	static SSImageList* create(const char* imageFilenames[], const char* imageDir = NULL);

	/** アニメーションデータから、このオブジェクトを初期化します.
	 *  Initialize from animation data.
	 */
	bool init(const SSData* ssData, const char* imageDir = NULL);

	/** ファイル名リストから、このオブジェクトを初期化します.
	 *  Initialize from filename list.
	 */
	bool init(const char* imageFilenames[], const char* imageDir = NULL);

	/** 指定インデックスのテクスチャを返します.
	 *  Get texture at specified index.
	 */
	cocos2d::CCTexture2D* getTexture(size_t index);

public:
	SSImageList(void);
	virtual ~SSImageList();

protected:
	/** 全てのテクスチャを削除します.
	 *  Remove all textures.
	 */
	void removeAll();
	
	/** テクスチャをロードし追加します.
	 *  Load texture and add.
	 */
	void addTexture(const char* imageName, const char* imageDir);

	cocos2d::CCArray	m_imageList;
};



/**
 * SSUserData
 */

struct SSUserData
{
	enum {
		FLAG_NUMBER		= 1 << 0,
		FLAG_RECT		= 1 << 1,
		FLAG_POINT		= 1 << 2,
		FLAG_STRING		= 1 << 3
	};

	int			flags;			// Flags of valid data
	ss_s32		number;			// Numeric
	ss_s32		rect[4];		// Rectangle Left, Top, Right, Bottom
	ss_s32		point[2];		// Position X, Y
	const char*	str;			// String (zero terminated)
	int			strSize;		// String size (byte count)
};



/**
 * SSPlayer
 */

class SSPlayer : public cocos2d::CCSprite
{
public:
	/** SSPlayerを生成します.
	 *  Create a SSPlayer object.
	 */
	static SSPlayer* create();

	/** SSPlayerを生成し、アニメーションを設定します.
	 *  Create a SSPlayer object, and set animation.
	 */
	static SSPlayer* create(const SSData* ssData, SSImageList* imageList);

	/** アニメーションを設定します.
	 *  Set animation.
	 */
	void setAnimation(const SSData* ssData, SSImageList* imageList);

	/** 再生フレームNoを取得します.
	 *  Get frame no of playing.
	 */
	int getFrameNo() const;

	/** 再生フレームNoを設定します.
	 *  Set frame no of playing.
	 */
	void setFrameNo(int frameNo);

	/** 再生スピードを取得します. (1.0f:標準)
	 *  Set speed to play. (1.0f:normal speed)
	 */
	float getStep() const;

	/** 再生スピードを設定します. (1.0f:標準)
	 *  Get speed to play. (1.0f:normal speed)
	 */
	void setStep(float step);
	
	/** 指定されている再生ループ回数を取得します. (0:指定なし)
	 *  Get a playback loop count of specified. (0:not specified)
	 */
	int getLoop() const;

	/** 再生ループ回数を設定します. (0:指定なし)
	 *  Set a playback loop count.  (0:not specified)
	 */
	void setLoop(int loop);

	/** 現在までのループ再生回数を取得します.
	 *  Get repeat count a playback.
	 */
	int getLoopCount() const;

	/** 現在までのループ再生回数をクリアします.
	 *  Clear repeat count a playback.
	 */
	void clearLoopCount();

	/** フレームスキップ（フレームレートに合わせ再生フレームをスキップする）の設定をします. (default: true)
	 *  Set frame-skip(to skip the playback frame according to the frame rate). (default: true)
	 */
	void setFrameSkipEnabled(bool enabled);
	
	/** フレームスキップの設定状態を返します.
	 *  Get frame-skip setting.
	 */
	bool isFrameSkipEnabled() const;

	/** ユーザーデータなどの通知を受け取る、デリゲートを設定します.
	 *  Set delegate. receive a notification, such as user data.
	 */
	void setDelegate(SSPlayerDelegate* delegate);


	/** パーツの状態を示します.
	 *  Indicates the status of the parts.
	 */
	struct PartState
	{
		float	x;
		float	y;
		float	scaleX;
		float	scaleY;
		float	rotation;
		cocos2d::CCSprite* sprite;
	};

	/** 指定パーツの情報をPartInfoに格納します.
	 *  成功時はtrueを、パーツが見つからないときはfalseを返します.
	 *  Get specified parts status, set to PartInfo.
	 *  Upon success, true is returned. otherwise false, parts not found.
	 */
	bool getPartState(PartState& result, const char* name);
	  
	virtual void	setFlipX(bool bFlipX);
	virtual void	setFlipY(bool bFlipY);
	virtual void	setScaleX(float fScaleX);
    virtual void	setScaleY(float fScaleY);
	virtual bool	isFlipX(void){ return m_ssPlayerFlipX; }
	virtual bool	isFlipY(void){ return m_ssPlayerFlipY; }
	virtual float	getScaleX();
	virtual float	getScaleY();
	virtual float	getScale();

public:
	SSPlayer(void);
	virtual ~SSPlayer();
	virtual bool init();
	void update(float dt);

protected:
	void allocParts(int numParts, bool useCustomShaderProgram);
	void releaseParts();

	void clearAnimation();
	bool hasAnimation() const;

	void updateFrame(float dt);
	void setFrame(int frameNo);
	void setChildVisibleAll(bool visible);
	void checkUserData(int frameNo);

	friend class SSPlayerBatch;

	void registerBatch(SSPlayerBatch* batch);
	void unregisterBatch(SSPlayerBatch* batch);

protected:
	class SSDataHandle*	m_ssDataHandle;
	SSImageList*		m_imageList;
	bool				m_frameSkipEnabled;
	SSPlayerDelegate*	m_delegate;
	SSUserData			m_userData;

	SSPlayerBatch*		m_batch;
	cocos2d::CCArray	m_batchSprites;
	cocos2d::CCArray	m_jointSprites;
	
	cocos2d::CCArray	m_partStates;
	float				m_playingFrame;
	float				m_step;
	int					m_loop;
	int					m_loopCount;

	float				m_ssPlayerScaleX;
	float				m_ssPlayerScaleY;
	bool				m_ssPlayerFlipX;
	bool				m_ssPlayerFlipY;
};



/**
 * SSPlayerDelegate
 */

class SSPlayerDelegate
{
public:
	SSPlayerDelegate(void) {}
	virtual ~SSPlayerDelegate();

	/** ユーザーデータの受信
	 *  Receive a user data.
	 */
	virtual void onUserData(SSPlayer* player, const SSUserData* data, int frameNo, const char* partName);
};



/**
 * SSPlayerBatch
 *
 * SSPlayerをSSPlayerBatch配下に配置することにより、できる限りバッチノードを使った描画を行うようになります。
 * 以下の条件を満たしているとき、バッチノードを使った描画が行われます。（条件に合わないパーツは単独で描画されます）
 * ・同一のテクスチャを使用している
 * ・次の効果を使用していない：頂点変形、カラーブレンド、αブレンドでMIX以外
 */

class SSPlayerBatch : public cocos2d::CCNode
{
public:
	static SSPlayerBatch* create();

	/** CCSpriteBatchNodeを構築する際に指定するcapacityを設定します
	 *  初期値はkDefaultSpriteBatchCapacityです
	 */
	void setDefaultSpriteBatchCapacity(unsigned int capacity);

	/** SSPlayerオブジェクトを登録する　※SSPlayer型のみ登録可能です
	 */
	virtual void addChild(CCNode * child);

	/** SSPlayerオブジェクトを登録する　※SSPlayer型のみ登録可能です
	 */
	virtual void addChild(CCNode * child, int zOrder);

	/** SSPlayerオブジェクトを登録する　※SSPlayer型のみ登録可能です
	 */
	virtual void addChild(CCNode * child, int zOrder, int tag);

	/** SSPlayerオブジェクトを除外する
	 */
	virtual void removeChild(CCNode * child);

public:
	SSPlayerBatch(void);
	virtual ~SSPlayerBatch();
	virtual bool init();
	void update(float dt);
	
	void getNode(cocos2d::CCNode*& node, bool batchNodeRequired, cocos2d::CCTexture2D* tex);

protected:
	cocos2d::CCNode* m_players;
	cocos2d::CCNode* m_bundles;
	unsigned int m_defaultCapacity;

	int m_currentNodeIndex;
	cocos2d::CCNode* m_currentNode;
	cocos2d::CCSpriteBatchNode* m_currentBatchNode;
	bool m_isBatchNodeCurrent;
	cocos2d::CCTexture2D* m_currentTexture;
};



/**
 * helper
 */

struct SSPlayerHelper
{
	/** ssbaファイルをロードします.
	 *  使用済みポインタは必ず delete[] で破棄してください.
	 *  Load ssba file.
	 *  A pointer used, must be discard with delete[].
	 */
	static unsigned char* loadFile(const char* ssbaPath, const char* dir = NULL);

	/** ssbaファイルからSSPlayer/SSImageListオブジェクトを構築します
	 *  outDataに返されるポインタの破棄には必ず delete[] を使用してください.
	 *  Create SSPlayer/SSImageList objects, from ssba file.
	 *  outData returned pointer used, must be discard with delete[].
	 */
	static void createFromFile(unsigned char** outData, SSPlayer** outPlayer, SSImageList** outImageList, const char* ssbaPath, const char* dir = NULL);
};


#endif	// __SS_PLAYER_H__
