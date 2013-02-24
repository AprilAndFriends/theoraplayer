import os, sys, shutil
# -------------------------
if len(sys.argv) < 2:
	print "ERROR: script must be called with a version string as argument."
	sys.exit(1)
# -------------------------
root         = "../"
theoraplayer = root + "theoraplayer/theoraplayer.xcodeproj"
workspace    = root + "theoraplayer_workspace.xcodeproj"
ogg          = root + "ogg/macosx/Ogg.xcodeproj"
vorbis       = root + "vorbis/macosx/Vorbis.xcodeproj"
theora       = root + "theora/macosx/Theora.xcodeproj"
tremor       = root + "tremor/Tremor.xcodeproj"

version      = sys.argv[1]
path         = "libtheoraplayer_sdk_" + version;

iOS_simulator = "iphonesimulator5.0"
# -------------------------
def build(project, target, configuration, sdk = ""):
	cwd = os.getcwd()
	objpath = cwd + "/build"
	productpath = cwd + "/build/products"
	if sdk != "": sdk = "-sdk " + sdk
	ret = os.system("xcodebuild -project %s -target \"%s\" -configuration %s OBJROOT=\"%s\" SYMROOT=\"%s\" %s" % (project, target, configuration, objpath, productpath, sdk))
	if ret != 0:
		print "ERROR while building target: " + target
		sys.exit(ret)

def buildMac(project, target_base):
	build(project, target_base,            "Release")
	build(project, target_base,            "Release-LLVM")

def buildiOS(project, target_base):
	build(project, target_base + " (iOS)", "Release")
	build(project, target_base + " (iOS)", "Release", iOS_simulator)
	build(project, target_base + " (iOS)", "Release-LLVM")
	build(project, target_base + " (iOS)", "Release-LLVM", iOS_simulator)

def buildAll(project, target_base):
	buildMac(project, target_base)
	buildiOS(project, target_base)

def buildDeps():
	buildAll(ogg, "ogg")
	buildMac(vorbis, "vorbis")
	buildAll(theora, "theora")
	buildiOS(tremor, "tremor")

def copyDeps():
	pass

def clean():
	if os.path.exists(path): shutil.rmtree(path)
	if os.path.exists('build'): shutil.rmtree('build')
# -------------------------
clean()
buildDeps()
copyDeps()


#build(theoraplayer, "theoraplayer (iOS Theora)", "Release")
#os.system("svn export libtheoraplayer_sdk/ " + path)


print "---------------------------"
print "Successfully compiled Theora Playback Library SDK: version " + version + " !"