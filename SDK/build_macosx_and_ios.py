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
gcc_path     = 'gcc4.2'
llvm_path    = 'llvm4.2'
iphoneos_path        = '/device_armv7_armv7s'
iphonesimulator_path = '/simulator_i386'

version      = sys.argv[1]
path         = "libtheoraplayer_sdk_" + version;

iOS_simulator = "iphonesimulator5.0"
# -------------------------
def clean():
	if os.path.exists(path): shutil.rmtree(path)
	if os.path.exists('build'): shutil.rmtree('build')
	os.system("svn export libtheoraplayer_sdk/ " + path)
	
def build(project, target, configuration, sdk = ""):
	cwd = os.getcwd()
	objpath = cwd + "/build"
	productpath = cwd + "/build/products"
	if sdk != "": sdk = "-sdk " + sdk
	ret = os.system("xcodebuild -project %s -target \"%s\" -configuration %s OBJROOT=\"%s\" SYMROOT=\"%s\" %s DEPLOYMENT_LOCATION=NO" % (project, target, configuration, objpath, productpath, sdk))
	if ret != 0:
		print "ERROR while building target: " + target
		sys.exit(ret)

def buildMac(project, target_base):
	build(project, target_base,            "Release")
	build(project, target_base,            "Release-LLVM")

def buildiOS(project, target_base, suffix = " (iOS)"):
	build(project, target_base + suffix, "Release")
	build(project, target_base + suffix, "Release", iOS_simulator)
	build(project, target_base + suffix, "Release-LLVM")
	build(project, target_base + suffix, "Release-LLVM", iOS_simulator)

def buildAll(project, target_base):
	buildMac(project, target_base)
	buildiOS(project, target_base)

def buildDeps():
	buildAll(ogg, "ogg")
	buildMac(vorbis, "vorbis")
	buildAll(theora, "theora")
	buildiOS(tremor, "tremor")

def copyDeps():
	deps_path = '/Dependencies'
	shutil.copytree('build/products/Release',                      path + '/lib/macosx-' + gcc_path  + deps_path)
	shutil.copytree('build/products/Release-LLVM',                 path + '/lib/macosx-' + llvm_path + deps_path)
	shutil.copytree('build/products/Release-iphonesimulator',      path + '/lib/ios-'    + gcc_path  + iphonesimulator_path + deps_path)
	shutil.copytree('build/products/Release-LLVM-iphonesimulator', path + '/lib/ios-'    + llvm_path + iphonesimulator_path + deps_path)
	shutil.copytree('build/products/Release-iphoneos',             path + '/lib/ios-'    + gcc_path  + iphoneos_path + deps_path)
	shutil.copytree('build/products/Release-LLVM-iphoneos',        path + '/lib/ios-'    + llvm_path + iphoneos_path + deps_path)

def copyHeaders():
	shutil.copytree('../ogg/include/ogg',                   path + '/include/ogg')
	shutil.copytree('../vorbis/include/vorbis',             path + '/include/vorbis')
	shutil.copytree('../theora/include/theora',             path + '/include/theora')
	shutil.copytree('../theoraplayer/include/theoraplayer', path + '/include/theoraplayer')

def rm(path):
	if os.path.exists(path):
		if os.path.isdir(path):
			shutil.rmtree(path)
		else:
			os.remove(path)

def makeTheoraplayerFramework(version):
	framework_path = '/theoraplayer.framework'
	build_path = 'build/products/Release'
	rm(build_path + framework_path)
	rm(build_path + '-LLVM' + framework_path)
	
	buildMac(theoraplayer, "theoraplayer (" + version + ")")
	shutil.copytree('build/products/Release'      + framework_path, path + '/lib/macosx-' + gcc_path  + "/" + version.replace(" ", "_") + framework_path)
	shutil.copytree('build/products/Release-LLVM' + framework_path, path + '/lib/macosx-' + llvm_path + "/" + version.replace(" ", "_") + framework_path)

def makeTheoraplayeriOSLib(version):
	lib_path = '/libtheoraplayer.a'
	build_path_sim_gcc = 'build/products/Release-iphonesimulator'
	build_path_dev_gcc = 'build/products/Release-iphoneos'
	build_path_sim_llvm = 'build/products/Release-LLVM-iphonesimulator'
	build_path_dev_llvm = 'build/products/Release-LLVM-iphoneos'
	rm(build_path_sim_gcc  + lib_path)
	rm(build_path_dev_gcc  + lib_path)
	rm(build_path_sim_llvm + lib_path)
	rm(build_path_dev_llvm + lib_path)
	
	buildiOS(theoraplayer, "theoraplayer", " (iOS " + version + ")")
	shutil.copyfile(build_path_dev_gcc  + lib_path, path + '/lib/ios-' + gcc_path   + "/" + iphoneos_path        + "/" + version.replace(" ", "_") + lib_path)
	shutil.copyfile(build_path_dev_llvm + lib_path, path + '/lib/ios-' + llvm_path  + "/" + iphoneos_path        + "/" + version.replace(" ", "_") + lib_path)
	shutil.copyfile(build_path_sim_gcc  + lib_path, path + '/lib/ios-' + gcc_path   + "/" + iphonesimulator_path + "/" + version.replace(" ", "_") + lib_path)
	shutil.copyfile(build_path_sim_llvm + lib_path, path + '/lib/ios-' + llvm_path  + "/" + iphonesimulator_path + "/" + version.replace(" ", "_") + lib_path)

def makeTheoraPlayerLibs():
	makeTheoraplayerFramework("Theora")
	makeTheoraplayerFramework("AVFoundation")
	makeTheoraplayerFramework("Theora AVFoundation")

	makeTheoraplayeriOSLib("Theora")
	makeTheoraplayeriOSLib("AVFoundation")
	makeTheoraplayeriOSLib("Theora AVFoundation")
# -------------------------
clean()
copyHeaders()
buildDeps()
copyDeps()
makeTheoraPlayerLibs()

print "---------------------------"
print "Successfully compiled Theora Playback Library SDK: version " + version + " !"