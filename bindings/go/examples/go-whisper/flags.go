package main

import (
	"flag"
)

///////////////////////////////////////////////////////////////////////////////
// TYPES

type Flags struct {
	*flag.FlagSet
}

///////////////////////////////////////////////////////////////////////////////
// LIFECYCLE

func NewFlags(name string, args []string) (*Flags, error) {
	flags := &Flags{
		FlagSet: flag.NewFlagSet(name, flag.ContinueOnError),
	}

	// Register the command line arguments
	registerFlags(flags)

	// Parse command line
	if err := flags.Parse(args); err != nil {
		return nil, err
	}

	// Return success
	return flags, nil
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS

func (flags *Flags) GetModel() string {
	return flags.Lookup("model").Value.String()
}

func (flags *Flags) GetLanguage() string {
	return flags.Lookup("language").Value.String()
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS

func registerFlags(flag *Flags) {
	flag.String("model", "", "Path to the model file")
	flag.String("language", "", "Language")
}
