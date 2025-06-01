package cjson

import (
	cbase "github.com/jurgen-kluft/cbase/package"
	denv "github.com/jurgen-kluft/ccode/denv"
	ccore "github.com/jurgen-kluft/ccore/package"
	cunittest "github.com/jurgen-kluft/cunittest/package"
)

// GetPackage returns the package object of 'cjson'
func GetPackage() *denv.Package {
	// Dependencies
	cunittestpkg := cunittest.GetPackage()
	cbasepkg := cbase.GetPackage()
	ccorepkg := ccore.GetPackage()

	// The main (cjson) package
	mainpkg := denv.NewPackage("github.com\\jurgen-kluft", "cjson")
	mainpkg.AddPackage(cunittestpkg)
	mainpkg.AddPackage(cbasepkg)
	mainpkg.AddPackage(ccorepkg)

	// 'cjson' library
	mainlib := denv.SetupCppLibProject(mainpkg, "cjson")
	mainlib.AddDependencies(cbasepkg.GetMainLib()...)
	mainlib.AddDependencies(ccorepkg.GetMainLib()...)

	// 'cjson' unittest project
	maintest := denv.SetupCppTestProject(mainpkg, "cjson_test")
	maintest.AddDependencies(cunittestpkg.GetMainLib()...)
	maintest.AddDependencies(cbasepkg.GetMainLib()...)
	maintest.AddDependencies(ccorepkg.GetMainLib()...)
	maintest.AddDependency(mainlib)

	mainpkg.AddMainLib(mainlib)
	mainpkg.AddUnittest(maintest)
	return mainpkg
}
