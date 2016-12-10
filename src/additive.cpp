#include <iostream>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include "visionkit.hpp"
#include "additive.hpp"

void forwardAdditiveImageAlign(cv::Mat& imgT, cv::Mat& imgI, cv::Rect& omega)
{
    const float EPS = 1E-5f; // Threshold value for termination criteria.
    const int MAX_ITER = 100;  // Maximum iteration count.

    const int cols = omega.width;
    const int rows = omega.height;

    std::cout << std::endl << "Start Forward Additive Algorithm!" << std::endl;

    clock_t start_time = clock();

    /*
    *  Precomputation stage.
    */

    cv::Mat T = imgT(omega).clone();

    //! Step1: Get gradient ▽I
    //! this ▽I is gradient of original image, not the 'I' we discuss which is rectangular eare of the original image
    cv::Mat gradIx, gradIy;
    gradient(imgI, gradIx, 1, 0);
    gradient(imgI, gradIy, 0, 1);

    /*
    *   Iteration stage.
    */

    //! Evaluate Model's Parameter in Warp: p
    cv::Mat p = cv::Mat::zeros(6, 1, CV_32FC1);
    cv::Mat A = cv::Mat::eye(3, 3, CV_32FC1);

    float mean_error = 0;
    int iter = 0;

    while(iter < MAX_ITER)
    {
        iter++;

        cv::Mat IW;
        cv::Mat gradIx_W, gradIy_W;
        cv::Mat H = cv::Mat::zeros(6, 6, CV_32FC1);
        cv::Mat Jres = cv::Mat::zeros(6, 1, CV_32FC1);
        cv::Mat dp = cv::Mat::zeros(6, 1, CV_32FC1);

        //! Step2: Get the Warp Image of I: I(W(x;p))
        warpAffine(imgI, IW, A, omega);

        //! Step3: Warp the gradient ▽I with W(x;p)
        warpAffine_float(gradIx, gradIx_W, A, omega);
        warpAffine_float(gradIy, gradIy_W, A, omega);

        cv::Mat jac;
        cv::Mat dxy;
        cv::Mat J;
        for(int y = 0; y < rows; ++y)
        {
            uint8_t* pIW = IW.ptr<uint8_t>(y);
            uint8_t* pT = T.ptr<uint8_t>(y);
            for(int x = 0; x < cols; ++x)
            {
                //! Step4: Evaluate the Jacobin ∂W/∂p at (x;p)
                jac = (cv::Mat_<float>(2, 6) << x, y, 0, 0, 1, 0, 0, 0, x, y, 0, 1);

                //! Step5: Calculate steepest descent image ▽I*∂W/∂p
                dxy = (cv::Mat_<float>(1, 2) << gradIx_W.at<float>(y, x), gradIy_W.at<float>(y, x));
                J = dxy*jac;

                //! Step6: Calculate Hessian Matrix H = ∑x[▽I*∂W/∂p]^T*[▽I*∂W/∂p]
                H += J.t() * J;

                //! Step7: Compute the error image T(x) - I(W(x:p))
                float res = pT[x] * 1.0 - pIW[x];
                mean_error += res*res;

                //! Step8: Compute Jres = ∑x[▽I*∂W/∂p]^T*[T(x)-I(W(x;p))]
                Jres += J.t() * res;
            }
        }

        mean_error /= rows*cols;

        //! Step9: Compute △p = H^(-1) * Jres
        dp = H.inv() * Jres;

        //! Step10: Update the parameters p = p + △p
        p += dp;
        float* pp = p.ptr<float>(0);
        A = (cv::Mat_<float>(3, 3) << 1 + *pp, *(pp + 1), *(pp + 4), *(pp + 2), 1 + *(pp + 3), *(pp + 5), 0, 0, 1);

#ifdef  DEBUG_INF_OUT
        std::cout << "A:" << A << std::endl;
        std::cout << "Iter:" << iter << "  ";
        std::cout << "Mean Error:" << mean_error << std::endl;
#endif // DEBUG_INF_OUT

        if (fabs(dp.at<float>(0, 0)) < EPS && fabs(dp.at<float>(1, 0)) < EPS && fabs(dp.at<float>(2, 0)) < EPS && fabs(dp.at<float>(3, 0)) < EPS && fabs(dp.at<float>(4, 0)) < EPS && fabs(dp.at<float>(5, 0)) < EPS)
        {break;}
    }
    clock_t finish_time = clock();
    double total_time = (double)(finish_time - start_time) / CLOCKS_PER_SEC;

    //! Print summary.
    std::cout << "===============================================" << std::endl;
    std::cout << "Algorithm: Forward Additive" << std::endl;
    std::cout << "A:" << std::endl << A << std::endl;
    std::cout << "Mean Error:" << mean_error << std::endl;
    std::cout << "Iteration:" << iter << std::endl;
    std::cout << "Total Time:" << total_time << std::endl;
    std::cout << "===============================================" << std::endl;

}