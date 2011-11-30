// This is brl/bpro/core/vil_pro/processes/vil_gradient_angle_process.cxx
#include <bprb/bprb_func_process.h>
//:
// \file

#include <vil/vil_math.h>
#include <vil/vil_convert.h>
#include <bprb/bprb_parameters.h>
#include <vil/vil_image_view_base.h>
#include <vgl/vgl_vector_2d.h>

//: Constructor
bool vil_gradient_angle_process_cons(bprb_func_process& pro)
{
  //input
  bool ok=false;
  vcl_vector<vcl_string> input_types;
  input_types.push_back("vil_image_view_base_sptr"); // dI/dx
  input_types.push_back("vil_image_view_base_sptr"); // dI/dy
  ok = pro.set_input_types(input_types);
  if (!ok) return ok;

  //output
  vcl_vector<vcl_string> output_types;
  output_types.push_back("vil_image_view_base_sptr"); // the angle of vector (dI/dx + dI/dy)
  ok = pro.set_output_types(output_types);
  if (!ok) return ok;
  return true;
}

//: Execute the process
bool vil_gradient_angle_process(bprb_func_process& pro)
{
  // Sanity check
  if (pro.n_inputs()< 2) {
    vcl_cout << "vil_gradient_angle_process: The input number should be 2" << vcl_endl;
    return false;
  }

  // get the inputs
  unsigned i=0;
  
  //Retrieve image from input
  vil_image_view_base_sptr dx_ptr = pro.get_input<vil_image_view_base_sptr>(i++);
  vil_image_view_base_sptr dy_ptr = pro.get_input<vil_image_view_base_sptr>(i++);

  // convert image to float
  vil_image_view<float> dx = *vil_convert_cast(float(), dx_ptr);
  vil_image_view<float> dy = *vil_convert_cast(float(), dy_ptr);
  //assert( dx.ni() != dy.ni() && dx.nj() != dy.nj()
  
  //calc angle
  vgl_vector_2d<float> horiz(1.0f, 0.0f); 
  vil_image_view<float>* theta = new vil_image_view<float>(dx.ni(), dy.ni()); 
  for(unsigned i=0; i<dx.ni(); ++i) {
    for(unsigned j=0; j<dx.nj(); ++j) {
      vgl_vector_2d<float> pvec(dx(i,j), dy(i,j)); 
      double t = angle( pvec, horiz);
      (*theta)(i,j) = (float) t; 
    } 
  }
      
  //set output
  pro.set_output_val<vil_image_view_base_sptr>(0, new vil_image_view<float>(theta));

  return true;
}
