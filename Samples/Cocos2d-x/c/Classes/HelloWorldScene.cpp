#include "HelloWorldScene.h"
#include "SSPlayerData.h"


//data.cppでincludeした.cファイルにある配列データです。
extern SSData attack_attack_partsData;
extern const char* attack_attack_images[];


USING_NS_CC;

CCScene* HelloWorld::scene()
{
    // 'scene' is an autorelease object
    CCScene *scene = CCScene::create();
    
    // 'layer' is an autorelease object
    HelloWorld *layer = HelloWorld::create();

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !CCLayer::init() )
    {
        return false;
    }
    
    CCSize visibleSize = CCDirector::sharedDirector()->getVisibleSize();
    CCPoint origin = CCDirector::sharedDirector()->getVisibleOrigin();

    /////////////////////////////
    // 2. add a menu item with "X" image, which is clicked to quit the program
    //    you may modify it.

    // add a "close" icon to exit the progress. it's an autorelease object
    CCMenuItemImage *pCloseItem = CCMenuItemImage::create(
                                        "CloseNormal.png",
                                        "CloseSelected.png",
                                        this,
                                        menu_selector(HelloWorld::menuCloseCallback));
    
	pCloseItem->setPosition(ccp(origin.x + visibleSize.width - pCloseItem->getContentSize().width/2 ,
                                origin.y + pCloseItem->getContentSize().height/2));

    // create menu, it's an autorelease object
    CCMenu* pMenu = CCMenu::create(pCloseItem, NULL);
    pMenu->setPosition(CCPointZero);
    this->addChild(pMenu, 1);

    /////////////////////////////
    // 3. add your codes below...

    // add a label shows "Hello World"
    // create and initialize a label
    
    CCLabelTTF* pLabel = CCLabelTTF::create("Hello World", "Arial", 24);
    
    // position the label on the center of the screen
    pLabel->setPosition(ccp(origin.x + visibleSize.width/2,
                            origin.y + visibleSize.height - pLabel->getContentSize().height));

    // add the label as a child to this layer
    this->addChild(pLabel, 1);

    // add "HelloWorld" splash screen"
    CCSprite* pSprite = CCSprite::create("HelloWorld.png");

    // position the sprite on the center of the screen
    pSprite->setPosition(ccp(visibleSize.width/2 + origin.x, visibleSize.height/2 + origin.y));

    // add the sprite as a child to this layer
    this->addChild(pSprite, 0);


    pLabelDebug = CCLabelTTF::create("DebugOut", "Arial", 24);
    pLabelDebug->setPosition(ccp(origin.x + visibleSize.width/2,
                            origin.y + visibleSize.height - pLabel->getContentSize().height - 24));
    this->addChild(pLabelDebug, 1);


	//アニメーションをC配列から定義します。
	m_ComipoImageList = SSImageList::create( attack_attack_images , "images\\" );
	m_ComipoAnime = SSPlayer::create( &attack_attack_partsData, m_ComipoImageList);

    

	//画面中心を取得
	int width = CCDirector::sharedDirector()->getWinSize().width;
	int height = CCDirector::sharedDirector()->getWinSize().height;


	this->addChild(m_ComipoAnime);

	int x = ( width / 2 );// - (w/2);
	int y = ( height / 2 );// + (h/2);

	m_ComipoAnime->setPositionX(x);
    m_ComipoAnime->setPositionY(y);
	m_ComipoAnime->setDelegate( this );

    return true;
}

//ユーザーデータの受け取りサンプル
void HelloWorld::onUserData(SSPlayer* player, const SSUserData* data, int frameNo, const char* partName)
{
	if ( data->flags == 0 ) return ;

	char buffer[256];
	sprintf( buffer, "frameNo = %d [%d,(%d,%d),(%d,%d,%d,%d),%s]" , frameNo  , 
		data->number , 
		data->point[0] , data->point[1], 
		data->rect[0] , data->rect[1] , data->rect[2] , data->rect[3] ,
		data->str );

	pLabelDebug->setString( buffer );
	pLabelDebug->draw();


}
void HelloWorld::menuCloseCallback(CCObject* pSender)
{
    CCDirector::sharedDirector()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}
