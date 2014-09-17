#ifndef __SS_PLAYER_DATA_H__
#define __SS_PLAYER_DATA_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef short			ss_s16;
typedef unsigned short	ss_u16;
typedef unsigned int	ss_u32;
typedef int				ss_s32;
typedef int				ss_offset;


typedef enum {
	kSSPartTypeNormal,
	kSSPartTypeNull
} SSPartType;


typedef enum {
	kSSPartAlphaBlendMix,
	kSSPartAlphaBlendMultiplication,
	kSSPartAlphaBlendAddition,
	kSSPartAlphaBlendSubtraction
} SSPartAlphaBlend;



typedef struct {
	ss_offset	partFrameData;
	ss_offset	userData;
	ss_s16		numParts;
	ss_s16		numUserData;
} SSFrameData;


typedef struct {
	ss_offset	name;
	ss_s16		id;
	ss_s16		parentId;
	ss_s16		imageNo;
	ss_u16		type;			// enum SSPartType
	ss_u16		alphaBlend;		// enum SSPartAlphaBlend
	ss_s16		reserved;
} SSPartData;


typedef struct {
	ss_u32		id[2];
	ss_u32		version;
	ss_u32		flags;
	ss_offset	partData;
	ss_offset	frameData;
	ss_offset	imageData;
	ss_s16		numParts;
	ss_s16		numFrames;
	ss_s16		fps;
} SSData;


#ifdef __cplusplus
}
#endif

#endif	// __SS_PLAYER_DATA_H__
