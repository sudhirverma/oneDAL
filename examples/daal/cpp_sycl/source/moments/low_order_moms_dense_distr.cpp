/* file: low_order_moms_dense_distr.cpp */
/*******************************************************************************
* Copyright 2020 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/*
!  Content:
!    C++ example of computing low order moments in the distributed processing
!    mode
!******************************************************************************/

/**
 * <a name="DAAL-EXAMPLE-CPP-LOW_ORDER_MOMENTS_DENSE_DISTRIBUTED">
 * \example low_order_moms_dense_distr.cpp
 */

#include "daal_sycl.h"
#include "service.h"
#include "service_sycl.h"

using namespace daal;
using namespace daal::algorithms;
using namespace daal::data_management;

/* Input data set parameters */
const size_t nBlocks = 4;

const std::string datasetFileNames[] = { "../data/distributed/covcormoments_dense_1.csv",
                                         "../data/distributed/covcormoments_dense_2.csv",
                                         "../data/distributed/covcormoments_dense_3.csv",
                                         "../data/distributed/covcormoments_dense_4.csv" };

low_order_moments::PartialResultPtr partialResult[nBlocks];
low_order_moments::ResultPtr result;

void computestep1Local(size_t i);
void computeOnMasterNode();

void printResults(const low_order_moments::ResultPtr& res);

int main(int argc, char* argv[]) {
    checkArguments(argc,
                   argv,
                   4,
                   &datasetFileNames[0],
                   &datasetFileNames[1],
                   &datasetFileNames[2],
                   &datasetFileNames[3]);
    for (const auto& deviceSelector : getListOfDevices()) {
        const auto& nameDevice = deviceSelector.first;
        const auto& device = deviceSelector.second;
        sycl::queue queue(device);
        std::cout << "Running on " << nameDevice << "\n\n";

        daal::services::SyclExecutionContext ctx(queue);
        services::Environment::getInstance()->setDefaultExecutionContext(ctx);

        for (size_t i = 0; i < nBlocks; i++) {
            computestep1Local(i);
        }

        computeOnMasterNode();

        printResults(result);
    }

    return 0;
}

void computestep1Local(size_t block) {
    /* Initialize FileDataSource<CSVFeatureManager> to retrieve the input data from a .csv file */
    FileDataSource<CSVFeatureManager> dataSource(datasetFileNames[block],
                                                 DataSource::doAllocateNumericTable,
                                                 DataSource::doDictionaryFromContext);

    /* Retrieve the data from the input file */
    dataSource.loadDataBlock();

    /* Create an algorithm to compute low order moments in the distributed processing mode using the default method */
    low_order_moments::Distributed<step1Local> algorithm;

    /* Set input objects for the algorithm */
    algorithm.input.set(low_order_moments::data, dataSource.getNumericTable());

    /* Compute partial low order moments estimates on nodes */
    algorithm.compute();

    /* Get the computed partial estimates */
    partialResult[block] = algorithm.getPartialResult();
}

void computeOnMasterNode() {
    /* Create an algorithm to compute low order moments in the distributed processing mode using the default method */
    low_order_moments::Distributed<step2Master> algorithm;

    /* Set input objects for the algorithm */
    for (size_t i = 0; i < nBlocks; i++) {
        algorithm.input.add(low_order_moments::partialResults, partialResult[i]);
    }

    /* Compute a partial low order moments estimate on the master node from the partial estimates on local nodes */
    algorithm.compute();

    /* Finalize the result in the distributed processing mode */
    algorithm.finalizeCompute();

    /* Get the computed low order moments */
    result = algorithm.getResult();
}

void printResults(const low_order_moments::ResultPtr& res) {
    printNumericTable(res->get(low_order_moments::minimum), "Minimum:");
    printNumericTable(res->get(low_order_moments::maximum), "Maximum:");
    printNumericTable(res->get(low_order_moments::sum), "Sum:");
    printNumericTable(res->get(low_order_moments::sumSquares), "Sum of squares:");
    printNumericTable(res->get(low_order_moments::sumSquaresCentered),
                      "Sum of squared difference from the means:");
    printNumericTable(res->get(low_order_moments::mean), "Mean:");
    printNumericTable(res->get(low_order_moments::secondOrderRawMoment),
                      "Second order raw moment:");
    printNumericTable(res->get(low_order_moments::variance), "Variance:");
    printNumericTable(res->get(low_order_moments::standardDeviation), "Standard deviation:");
    printNumericTable(res->get(low_order_moments::variation), "Variation:");
}
