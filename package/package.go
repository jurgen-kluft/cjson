package xjson

import (
	xbase "github.com/jurgen-kluft/xbase/package"
	"github.com/jurgen-kluft/xcode/denv"
	xunittest "github.com/jurgen-kluft/xunittest/package"
)

// GetPackage returns the package object of 'xjson'
func GetPackage() *denv.Package {
	// Dependencies
	xunittestpkg := xunittest.GetPackage()
	xbasepkg := xbase.GetPackage()

	// The main (xjson) package
	mainpkg := denv.NewPackage("xjson")
	mainpkg.AddPackage(xunittestpkg)
	mainpkg.AddPackage(xbasepkg)

	// 'xjson' library
	mainlib := denv.SetupDefaultCppLibProject("xjson", "github.com\\jurgen-kluft\\xjson")
	mainlib.Dependencies = append(mainlib.Dependencies, xbasepkg.GetMainLib())

	// 'xjson' unittest project
	maintest := denv.SetupDefaultCppTestProject("xjson_test", "github.com\\jurgen-kluft\\xjson")
	maintest.Dependencies = append(maintest.Dependencies, xunittestpkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, xbasepkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, mainlib)

	mainpkg.AddMainLib(mainlib)
	mainpkg.AddUnittest(maintest)
	return mainpkg
}
