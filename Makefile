CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

waste_tracker: main.cpp utils.h waste_types.h analytics.h tracker.h
	$(CXX) $(CXXFLAGS) main.cpp -o waste_tracker

test: waste_tracker
	./waste_tracker --test

clean:
	rm -f waste_tracker waste_data.csv waste_config.txt waste_report.txt
