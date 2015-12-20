#include "r3dPCH.h"
#include "r3d.h"

#include "GameObjects\objmanag.h"
#include "gameobjects\obj_Dummy.h"

// temporary server object that will immidiatly destroy itself
class ServerDummyObject : public GameObject
{
  public:
	  virtual int			IsStatic() { return true; }
virtual	BOOL		Update() {
	  return FALSE;
	}
};


#define DEFINE_DUMMY_OBJECT(CLASS, NAME); \
  class CLASS : public ServerDummyObject \
  { \
	DECLARE_CLASS(CLASS, ServerDummyObject) \
  }; \
  \
  IMPLEMENT_CLASS(CLASS, NAME, "Object"); \
  AUTOREGISTER_CLASS(CLASS); \
  


//
// define and register object classes that is not used on server
//
DEFINE_DUMMY_OBJECT(obj_Waterfall, "obj_Waterfall");
DEFINE_DUMMY_OBJECT(obj_LampBulb,  "obj_LampBulb");
DEFINE_DUMMY_OBJECT(obj_AnimatedBuilding, "obj_AnimatedBuilding");
DEFINE_DUMMY_OBJECT(obj_Occluder,  "obj_Occluder");
DEFINE_DUMMY_OBJECT(obj_PEmitter,  "obj_PEmitter");
DEFINE_DUMMY_OBJECT(obj_ParticleSystem, "obj_ParticleSystem");
DEFINE_DUMMY_OBJECT(obj_EnvmapProbe, "obj_EnvmapProbe");
DEFINE_DUMMY_OBJECT(obj_Road, "obj_Road");
DEFINE_DUMMY_OBJECT(obj_Cloud, "obj_Cloud");
DEFINE_DUMMY_OBJECT(obj_LightHelper, "obj_LightHelper");
DEFINE_DUMMY_OBJECT(obj_AmbientSound, "obj_AmbientSound");
DEFINE_DUMMY_OBJECT(obj_AmbientSound2D, "obj_AmbientSound2D");
DEFINE_DUMMY_OBJECT(obj_MusicTriggerArea, "obj_MusicTriggerArea");
DEFINE_DUMMY_OBJECT(CloudGameObjectProxy, "CloudGameObjectProxy");
DEFINE_DUMMY_OBJECT(obj_ReverbZone, "obj_ReverbZone");
DEFINE_DUMMY_OBJECT(obj_ReverbZoneBox, "obj_ReverbZoneBox");
DEFINE_DUMMY_OBJECT(obj_PermanentNote, "obj_PermanentNote");

DEFINE_DUMMY_OBJECT(obj_DeerDummy, "obj_DeerDummy");
DEFINE_DUMMY_OBJECT(obj_ZombieDummy, "obj_ZombieDummy");
DEFINE_DUMMY_OBJECT(obj_AnimalDummy, "obj_AnimalDummy");

// fucking dummy object.
DummyObject::DummyObject() {}
DummyObject::~DummyObject() {}
BOOL DummyObject::Load(const char* name) { return parent::Load(name); }
void DummyObject::AppendShadowRenderables( RenderArray&  rarr, int sliceIndex, const r3dCamera& Cam ) {} 
void DummyObject::AppendRenderables( RenderArray ( & render_arrays  )[ rsCount ], const r3dCamera& Cam ) {}
BOOL DummyObject::Update() { return TRUE; }
