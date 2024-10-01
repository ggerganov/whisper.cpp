package main

import (
	"context"
	"flag"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"os"
	"path/filepath"
	"syscall"
	"time"
)

///////////////////////////////////////////////////////////////////////////////
// CONSTANTS

const (
	srcUrl  = "https://huggingface.co/ggerganov/whisper.cpp/resolve/main" // The location of the models
	srcExt  = ".bin"                                                      // Filename extension
	bufSize = 1024 * 64                                                   // Size of the buffer used for downloading the model
)

var (
	// The models which will be downloaded, if no model is specified as an argument
	modelNames = []string{"ggml-tiny.en", "ggml-tiny", "ggml-base.en", "ggml-base", "ggml-small.en", "ggml-small", "ggml-medium.en", "ggml-medium", "ggml-large-v1", "ggml-large-v2", "ggml-large-v3", "large-v3-turbo"}
)

var (
	// The output folder. When not set, use current working directory.
	flagOut = flag.String("out", "", "Output folder")

	// HTTP timeout parameter - will timeout if takes longer than this to download a model
	flagTimeout = flag.Duration("timeout", 30*time.Minute, "HTTP timeout")

	// Quiet parameter - will not print progress if set
	flagQuiet = flag.Bool("quiet", false, "Quiet mode")
)

///////////////////////////////////////////////////////////////////////////////
// MAIN

func main() {
	flag.Usage = func() {
		name := filepath.Base(flag.CommandLine.Name())
		fmt.Fprintf(flag.CommandLine.Output(), "Usage: %s [options] <model>\n\n", name)
		flag.PrintDefaults()
	}
	flag.Parse()

	// Get output path
	out, err := GetOut()
	if err != nil {
		fmt.Fprintln(os.Stderr, "Error:", err)
		os.Exit(-1)
	}

	// Create context which quits on SIGINT or SIGQUIT
	ctx := ContextForSignal(os.Interrupt, syscall.SIGQUIT)

	// Progress filehandle
	progress := os.Stdout
	if *flagQuiet {
		progress, err = os.Open(os.DevNull)
		if err != nil {
			fmt.Fprintln(os.Stderr, "Error:", err)
			os.Exit(-1)
		}
		defer progress.Close()
	}

	// Download models - exit on error or interrupt
	for _, model := range GetModels() {
		url, err := URLForModel(model)
		if err != nil {
			fmt.Fprintln(os.Stderr, "Error:", err)
			continue
		} else if path, err := Download(ctx, progress, url, out); err == nil || err == io.EOF {
			continue
		} else if err == context.Canceled {
			os.Remove(path)
			fmt.Fprintln(progress, "\nInterrupted")
			break
		} else if err == context.DeadlineExceeded {
			os.Remove(path)
			fmt.Fprintln(progress, "Timeout downloading model")
			continue
		} else {
			os.Remove(path)
			fmt.Fprintln(os.Stderr, "Error:", err)
			break
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS

// GetOut returns the path to the output directory
func GetOut() (string, error) {
	if *flagOut == "" {
		return os.Getwd()
	}
	if info, err := os.Stat(*flagOut); err != nil {
		return "", err
	} else if !info.IsDir() {
		return "", fmt.Errorf("not a directory: %s", info.Name())
	} else {
		return *flagOut, nil
	}
}

// GetModels returns the list of models to download
func GetModels() []string {
	if flag.NArg() == 0 {
		return modelNames
	} else {
		return flag.Args()
	}
}

// URLForModel returns the URL for the given model on huggingface.co
func URLForModel(model string) (string, error) {
	if filepath.Ext(model) != srcExt {
		model += srcExt
	}
	url, err := url.Parse(srcUrl)
	if err != nil {
		return "", err
	} else {
		url.Path = filepath.Join(url.Path, model)
	}
	return url.String(), nil
}

// Download downloads the model from the given URL to the given output directory
func Download(ctx context.Context, p io.Writer, model, out string) (string, error) {
	// Create HTTP client
	client := http.Client{
		Timeout: *flagTimeout,
	}

	// Initiate the download
	req, err := http.NewRequest("GET", model, nil)
	if err != nil {
		return "", err
	}
	resp, err := client.Do(req)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return "", fmt.Errorf("%s: %s", model, resp.Status)
	}

	// If output file exists and is the same size as the model, skip
	path := filepath.Join(out, filepath.Base(model))
	if info, err := os.Stat(path); err == nil && info.Size() == resp.ContentLength {
		fmt.Fprintln(p, "Skipping", model, "as it already exists")
		return "", nil
	}

	// Create file
	w, err := os.Create(path)
	if err != nil {
		return "", err
	}
	defer w.Close()

	// Report
	fmt.Fprintln(p, "Downloading", model, "to", out)

	// Progressively download the model
	data := make([]byte, bufSize)
	count, pct := int64(0), int64(0)
	ticker := time.NewTicker(5 * time.Second)
	for {
		select {
		case <-ctx.Done():
			// Cancelled, return error
			return path, ctx.Err()
		case <-ticker.C:
			pct = DownloadReport(p, pct, count, resp.ContentLength)
		default:
			// Read body
			n, err := resp.Body.Read(data)
			if err != nil {
				DownloadReport(p, pct, count, resp.ContentLength)
				return path, err
			} else if m, err := w.Write(data[:n]); err != nil {
				return path, err
			} else {
				count += int64(m)
			}
		}
	}
}

// Report periodically reports the download progress when percentage changes
func DownloadReport(w io.Writer, pct, count, total int64) int64 {
	pct_ := count * 100 / total
	if pct_ > pct {
		fmt.Fprintf(w, "  ...%d MB written (%d%%)\n", count/1e6, pct_)
	}
	return pct_
}
