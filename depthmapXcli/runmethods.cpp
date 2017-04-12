// Copyright (C) 2017 Christian Sailer

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "runmethods.h"
#include "salalib/mgraph.h"
#include "radiusconverter.h"
#include "exceptions.h"
#include "simpletimer.h"
#include <memory>
#include <sstream>
#include <vector>

namespace dm_runmethods
{

    vector<PixelRefPair> getLinksFromMergeLines(const vector<Line>& merge_lines, PointMap& current_map)
    {
        vector<PixelRefPair> merge_pixel_pairs;
        for (size_t i = 0; i < merge_lines.size(); i++)
        {
            const Line & merge_line = merge_lines.at(i);
            const PixelRef & a = current_map.pixelate(merge_line.start());
            const PixelRef & b = current_map.pixelate(merge_line.end());

            // check in limits:
            if (!current_map.includes(a) || !current_map.getPoint(a).filled()
                    || !current_map.includes(b) || !current_map.getPoint(b).filled())
            {
                std::stringstream message;
                message << "Line ends not both on painted analysis space "
                        << i << " ("
                        << merge_line.start().x << ", "
                        << merge_line.start().y << " -> "
                        << merge_line.end().x << ", "
                        << merge_line.end().y << ")" << flush;
                throw depthmapX::RuntimeException(message.str().c_str());
            }

            // we probably need to check if we were given coordinates that
            // fall on a previously given cell, in which case the newest given
            // will replace the oldest and effectively delete the whole link
            for (size_t j = 0; j < merge_pixel_pairs.size(); j++)
            {
                // PixelRefPair internal == operator only checks a with a and b with b
                // but we also need to check the inverse
                if(a == merge_pixel_pairs.at(j).a
                        || b == merge_pixel_pairs.at(j).b
                        || a == merge_pixel_pairs.at(j).b
                        || b == merge_pixel_pairs.at(j).a)
                {
                    // one of the cells has already been seen.
                    std::stringstream message;
                    message << "Overlapping link found at line "
                            << i << " ("
                            << merge_line.start().x << ", "
                            << merge_line.start().y << " -> "
                            << merge_line.end().x << ", "
                            << merge_line.end().y << ")" << flush;
                    throw depthmapX::RuntimeException(message.str().c_str());
                }
            }

            // TODO: the merge function will replace any links that already exist
            // on the two locations, so we need to warn the user if this is the case

            merge_pixel_pairs.push_back(PixelRefPair(a, b));
        }
        return merge_pixel_pairs;
    }

    MetaGraph loadGraph(const pstring& filename) {
        MetaGraph mgraph;
        auto result = mgraph.read(filename);
        if ( result != MetaGraph::OK)
        {
            std::stringstream message;
            message << "Failed to load graph from file " << filename << ", error " << result << flush;
            throw depthmapX::RuntimeException(message.str().c_str());
        }
        return mgraph;
    }

    void linkGraph(const CommandLineParser &cmdP)
    {
        MetaGraph mgraph = loadGraph(cmdP.getFileName().c_str());
        PointMap& current_map = mgraph.getDisplayedPointMap();

        vector<PixelRefPair> new_links = getLinksFromMergeLines(cmdP.linkOptions().getMergeLines(), current_map);

        for (size_t i = 0; i < new_links.size(); i++)
        {
            PixelRefPair link = new_links.at(i);
            current_map.mergePixels(link.a,link.b);
        }
        mgraph.write(cmdP.getOuputFile().c_str(),METAGRAPH_VERSION, false);
    }

    void runVga(const CommandLineParser &cmdP, const IRadiusConverter &converter)
    {
        MetaGraph mgraph = loadGraph(cmdP.getFileName().c_str());

        std::unique_ptr<Communicator> comm(new ICommunicator());
        std::unique_ptr<Options> options(new Options());

        switch(cmdP.vgaOptions().getVgaMode())
        {
            case VgaParser::VgaMode::VISBILITY:
                options->output_type = Options::OUTPUT_VISUAL;
                options->local = cmdP.vgaOptions().localMeasures();
                options->global = cmdP.vgaOptions().globalMeasures();
                if (options->global )
                {
                    options->radius = converter.ConvertForVisibility(cmdP.vgaOptions().getRadius());
                }
                break;
            case VgaParser::VgaMode::METRIC:
                options->output_type = Options::OUTPUT_METRIC;
                options->radius = converter.ConvertForMetric(cmdP.vgaOptions().getRadius());
                break;
            case VgaParser::VgaMode::ANGULAR:
                options->output_type = Options::OUTPUT_ANGULAR;
                break;
            case VgaParser::VgaMode::ISOVIST:
                options->output_type = Options::OUTPUT_ISOVIST;
            default:
                throw depthmapX::SetupCheckException("Unsupported VGA mode");
        }
        SimpleTimer timer;
        mgraph.analyseGraph(comm.get(), *options, cmdP.simpleMode() );
        std::cout << "Analysis took " << timer.getTimeInSeconds() << " seconds." << std::endl;
        mgraph.write(cmdP.getOuputFile().c_str(),METAGRAPH_VERSION, false);
    }

}
