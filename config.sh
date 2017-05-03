function run_tests {
    python -c "import segyio; print(segyio.__version__)"
    cp -r ../test-data .
    python -m unittest discover -vs ../python/test -p "*.py"
}
