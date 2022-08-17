package main

import (
	"github.com/jurgen-kluft/ccode"
	pkg "github.com/jurgen-kluft/cjson/package"
)

func main() {
	ccode.Init()
	ccode.Generate(pkg.GetPackage())
}
