/*
	gr-scan - A GNU Radio signal scanner
	Copyright (C) 2012  Nicholas Tomlinson
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>. 
*/


#include <cmath>
#include <stdint.h>

#include <gr_top_block.h>
#include <osmosdr_source_c.h>
#include <gr_stream_to_vector.h>
#include <gr_fft_vcc_fftw.h>
#include <gr_complex_to_xxx.h>
#include <gr_single_pole_iir_filter_ff.h>
#include <gr_nlog10_ff.h>
#include "scanner_sink.hpp"
#include <string>

class TopBlock : public gr_top_block
{
	public:
		TopBlock(double centre_freq_1, double centre_freq_2, double sample_rate, double fft_width, double bandwidth1, double bandwidth2,
				double step, unsigned int avg_size, double spread, double threshold, double ptime, std::string fileName) :
			gr_top_block("Top Block"),
			/*vector_length(sample_rate/fft_width),*/  // This doesn't make any sense to me, the vectorLength should just be the given fftWidth
			vector_length(fft_width),
			window(GetWindow(vector_length)),
			source(osmosdr_make_source_c()), /* OsmoSDR Source */
			stv(gr_make_stream_to_vector(sizeof(float)*2, vector_length)), /* Stream to vector */
			/* Based on the logpwrfft (a block implemented in python) */
			fft(gr_make_fft_vcc_fftw(vector_length, true, window, false, 1)),
			ctf(gr_make_complex_to_mag_squared(vector_length)),
			iir(gr_make_single_pole_iir_filter_ff(1.0, vector_length)),
			lg(gr_make_nlog10_ff(10, vector_length, -20 * log10(vector_length) -10 * log10(GetWindowPower()/vector_length))),
			/* Sink - this does most of the interesting work */
			sink(make_scanner_sink(source, vector_length, centre_freq_1, centre_freq_2, sample_rate, bandwidth1, bandwidth2, step, avg_size, spread, threshold, ptime, fileName))
		{

			printRunParams(centre_freq_1, centre_freq_2, sample_rate, fft_width, bandwidth1, bandwidth2, step, avg_size, spread, threshold, ptime);

			/* Set up the OsmoSDR Source */
			source->set_sample_rate(sample_rate);
			source->set_center_freq(centre_freq_1);
			source->set_freq_corr(0.0);
			source->set_gain_mode(false);
			source->set_gain(30);
			source->set_if_gain(25.0);
			
			/* Set up the connections */
			connect(source, 0, stv, 0);
			connect(stv, 0, fft, 0);
			connect(fft, 0, ctf, 0);
			connect(ctf, 0, iir, 0);
			connect(iir, 0, lg, 0);
			connect(lg, 0, sink, 0);
		}
		
	private:
		/* http://en.wikipedia.org/w/index.php?title=Window_function&oldid=508445914 */
		std::vector<float> GetWindow(size_t n)
		{
			std::vector<float> w;
			w.resize(n);

			double a = 0.16;
			double a0 = (1.0 - a)/2.0;
			double a1 = 0.5;
			double a2 = a/2.0;

			for (unsigned int i = 0; i < n; i++){
				w[i] = a0 - a1 * cos((2.0 * 3.14159 * (double)i)/(double)(n - 1)) + a2 * cos((4.0 * 3.14159 * (double)i)/(double)(n - 1));
			}
			return w;
		}

		double GetWindowPower()
		{
			double total = 0.0;
			std::vector<float>::iterator it = window.begin();
			for (it = window.begin(); it != window.end(); it++) {
				total += (*it) * (*it);
			}
			return total;
		}

		void printRunParams(double centerFreq1, double centerFreq2, double sampleRate, double fftWidth, double narrowBW, double courseBW, double stepSize, unsigned int averageSize, double spreadSize, double thresholdDB, double pauseTime)
		{
			printf("Start Freq: \t%f\n", centerFreq1);
			printf("Stop Freq: \t%f\n", centerFreq2);
			printf("Sample Rate: \t%f\n", sampleRate);
			printf("FFT Width: \t%f\n", fftWidth);
			printf("Narrow BW: \t%f\n", narrowBW);
			printf("Course BW: \t%f\n", courseBW);
			printf("Step Size: \t%f\n", stepSize);
			printf("FFT Average Size: \t%i\n", averageSize);
			printf("Spread delta: \t%f\n", spreadSize);
			printf("Threshold: \t%f\n", thresholdDB);
			printf("Pause Time: \t%f\n", pauseTime);
		}

		size_t vector_length;
		std::vector<float> window;

		osmosdr_source_c_sptr source;
		gr_stream_to_vector_sptr stv;
		gr_fft_vcc_sptr fft;
		gr_complex_to_mag_squared_sptr ctf;
		gr_single_pole_iir_filter_ff_sptr iir;
		gr_nlog10_ff_sptr lg;
		scanner_sink_sptr sink;
};
