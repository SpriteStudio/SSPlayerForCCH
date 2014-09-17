#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"
#include "SSPlayer.h"

class HelloWorld : public cocos2d::CCLayer , public SSPlayerDelegate
{
public:
    // Here's a difference. Method 'init' in cocos2d-x returns bool, instead of returning 'id' in cocos2d-iphone
    virtual bool init();  

    // there's no 'id' in cpp, so we recommend returning the class instance pointer
    static cocos2d::CCScene* scene();
    
    // a selector callback
    void menuCloseCallback(CCObject* pSender);
    
    // implement the "static node()" method manually
    CREATE_FUNC(HelloWorld);

public:
    SSImageList*	m_ComipoImageList;
	SSPlayer*		m_ComipoAnime;
	cocos2d::CCLabelTTF* pLabelDebug;

    virtual void onUserData(SSPlayer* player, const SSUserData* data, int frameNo, const char* partName);

};

#endif // __HELLOWORLD_SCENE_H__
