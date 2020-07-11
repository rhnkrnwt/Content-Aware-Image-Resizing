#include "gl_form.h"

using namespace System;
using namespace System::Windows::Forms;

namespace ContentAwareRetargeting {

  [STAThread]
  int main(array<String^> ^args) {
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);
    ContentAwareRetargeting::GLForm form;
    Application::Run(%form);
    return 0;
  }

  GLForm::GLForm() {
    InitializeComponent();

    //GroundTruthData();

    InitializeOpenGL();

    srand((unsigned)time(0));

    ChangeProgramStatus(IDLE);

    open_image_file_tool_strip_menu_item_->Click += gcnew System::EventHandler(this, &ContentAwareRetargeting::GLForm::OnButtonsClick);
    open_video_file_tool_strip_menu_item_->Click += gcnew System::EventHandler(this, &ContentAwareRetargeting::GLForm::OnButtonsClick);
    save_screen_tool_strip_menu_item_->Click += gcnew System::EventHandler(this, &ContentAwareRetargeting::GLForm::OnButtonsClick);
    start_button_->Click += gcnew System::EventHandler(this, &ContentAwareRetargeting::GLForm::OnButtonsClick);

    is_dragging_panel = false;

    gl_panel_->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &ContentAwareRetargeting::GLForm::OnMouseDown);
    gl_panel_->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &ContentAwareRetargeting::GLForm::OnMouseUp);
    gl_panel_->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &ContentAwareRetargeting::GLForm::OnMouseMove);

    this->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &ContentAwareRetargeting::GLForm::OnKeyDown);

    grid_size_track_bar_->ValueChanged += gcnew System::EventHandler(this, &ContentAwareRetargeting::GLForm::OnTrackBarsValueChanged);
    grid_size_track_bar_->Value = current_grid_size;

    focus_scale_track_bar_->ValueChanged += gcnew System::EventHandler(this, &ContentAwareRetargeting::GLForm::OnTrackBarsValueChanged);
    focus_scale_track_bar_->Value = focus_scale_track_bar_->Value + 1; // Trigger value changed event to rename the label

    show_image_check_box_->CheckedChanged += gcnew System::EventHandler(this, &ContentAwareRetargeting::GLForm::OnCheckBoxesCheckedChanged);
    show_lines_check_box_->CheckedChanged += gcnew System::EventHandler(this, &ContentAwareRetargeting::GLForm::OnCheckBoxesCheckedChanged);
    focus_check_box_->CheckedChanged += gcnew System::EventHandler(this, &ContentAwareRetargeting::GLForm::OnCheckBoxesCheckedChanged);

    target_width_numeric_up_down_->Minimum = MIN_PANEL_WIDTH;
    target_width_numeric_up_down_->Maximum = MAX_PANEL_WIDTH;

    target_height_numeric_up_down_->Minimum = MIN_PANEL_HEIGHT;
    target_height_numeric_up_down_->Maximum = MAX_PANEL_HEIGHT;

    target_width_numeric_up_down_->ValueChanged += gcnew System::EventHandler(this, &ContentAwareRetargeting::GLForm::OnNumericUpDownValueChanged);
    target_height_numeric_up_down_->ValueChanged += gcnew System::EventHandler(this, &ContentAwareRetargeting::GLForm::OnNumericUpDownValueChanged);
  }

  void GLForm::InitializeOpenGL() {

    // Get Handle
    hwnd = (HWND)gl_panel_->Handle.ToPointer();
    hdc = GetDC(hwnd);
    wglSwapBuffers(hdc);

    // Set pixel format
    PIXELFORMATDESCRIPTOR pfd;
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = (byte)(PFD_TYPE_RGBA);
    pfd.cColorBits = 32;
    pfd.cDepthBits = 32;
    pfd.iLayerType = (byte)(PFD_MAIN_PLANE);

    SetPixelFormat(hdc, ChoosePixelFormat(hdc, &pfd), &pfd);

    // Create OpenGL Rendering Context
    hrc = (wglCreateContext(hdc));
    if (!hrc) {
      std::cerr << "wglCreateContext failed.\n";
    }

    // Assign OpenGL Rendering Context
    if (!wglMakeCurrent(hdc, hrc)) {
      std::cerr << "wglMakeCurrent failed.\n";
    }

    if (glewInit() != GLEW_OK) {
      std::cerr << "OpenGL initialize failed.\n";
    }

    if (!GLEW_VERSION_2_0) {
      std::cerr << "Your graphic card does not support OpenGL 2.0.\n";
    }

    glewExperimental = GL_TRUE;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    gl_shader.CreateDefaultShader();
  }

  void GLForm::RenderGLPanel() {
    //wglMakeCurrent(hdc, hrc);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect_ratio = gl_panel_->Width / (float)gl_panel_->Height;

    glm::mat4 projection_matrix = glm::perspective(glm::radians(FOVY), aspect_ratio, 0.01f, 10000.0f);

    glm::mat4 view_matrix = glm::lookAt(eye_position * eye_position_scale, look_at_position, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 modelview_matrix = glm::mat4(1.0f);

    glUseProgram(gl_shader.shader_program_id_);

    glUniformMatrix4fv(gl_shader.shader_uniform_projection_matrix_id_, 1, GL_FALSE, glm::value_ptr(projection_matrix));
    glUniformMatrix4fv(gl_shader.shader_uniform_view_matrix_id_, 1, GL_FALSE, glm::value_ptr(view_matrix));

    if (is_demo_triangle) {

      if (show_lines_check_box_->Checked) {
        gl_panel_image_triangle_mesh.texture_flag_ = false;
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        gl_panel_image_triangle_mesh.Draw(gl_shader, modelview_matrix);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        gl_panel_image_triangle_mesh.texture_flag_ = true;
      }

      if (show_image_check_box_->Checked) {
        gl_panel_image_triangle_mesh.Draw(gl_shader, modelview_matrix);
      }

    } else {

      if (show_lines_check_box_->Checked) {
        gl_panel_image_mesh.texture_flag_ = false;
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        gl_panel_image_mesh.Draw(gl_shader, modelview_matrix);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        gl_panel_image_mesh.texture_flag_ = true;
      }

      if (show_image_check_box_->Checked) {
        gl_panel_image_mesh.Draw(gl_shader, modelview_matrix);
      }
    }

    SwapBuffers(hdc);
  }

  void GLForm::ChangeProgramStatus(const ProgramStatus &new_program_status) {
    program_status = new_program_status;
  }

  void GLForm::SaveGLScreen(const std::string &file_path) {
    std::vector<unsigned char> screen_image_data(3 * gl_panel_->Width * gl_panel_->Height);

    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, gl_panel_->Width, gl_panel_->Height, GL_BGR, GL_UNSIGNED_BYTE, &screen_image_data[0]);

    cv::Mat screen_image(gl_panel_->Height, gl_panel_->Width, CV_8UC3, &screen_image_data[0]);
    cv::flip(screen_image, screen_image, 0);
    cv::imwrite(file_path, screen_image);
    std::cout << "Screen saved : " << file_path << "\n";
  }

  void GLForm::SaveGLScreenToVideo(cv::VideoWriter &video_writer) {
    std::vector<unsigned char> screen_image_data(3 * gl_panel_->Width * gl_panel_->Height);

    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, gl_panel_->Width, gl_panel_->Height, GL_BGR, GL_UNSIGNED_BYTE, &screen_image_data[0]);
    cv::Mat screen_image(gl_panel_->Height, gl_panel_->Width, CV_8UC3, &screen_image_data[0]);
    cv::flip(screen_image, screen_image, 0);

    video_writer.write(screen_image);
  }

  void GLForm::ChangeGLPanelSize(int new_panel_width, int new_panel_height) {
    new_panel_width = std::max(MIN_PANEL_WIDTH, new_panel_width);
    new_panel_height = std::max(MIN_PANEL_HEIGHT, new_panel_height);

    new_panel_width = std::min(MAX_PANEL_WIDTH, new_panel_width);
    new_panel_height = std::min(MAX_PANEL_HEIGHT, new_panel_height);

    target_size_label_->Text = "Target size : " + new_panel_width + " X " + new_panel_height;

    gl_panel_->Width = new_panel_width;
    gl_panel_->Height = new_panel_height;

    glViewport(0, 0, gl_panel_->Width, gl_panel_->Height);

    double cotanget_of_half_of_fovy = 1.0 / tan(glm::radians(FOVY / 2.0f));

    eye_position = glm::vec3(new_panel_width / 2.0f, new_panel_height / 2.0f, cotanget_of_half_of_fovy * (new_panel_height / 2.0));
    look_at_position = glm::vec3(new_panel_width / 2.0f, new_panel_height / 2.0f, 0);

    RenderGLPanel();
  }

  void GLForm::BuildGridMeshAndGraphForImage(const cv::Mat &image, GLMesh &target_mesh, Graph<glm::vec2> &target_graph, float grid_size) {
    target_graph = Graph<glm::vec2>();

    size_t mesh_column_count = (size_t)(image.size().width / grid_size) + 1;
    size_t mesh_row_count = (size_t)(image.size().height / grid_size) + 1;

    float real_mesh_width = image.size().width / (float)(mesh_column_count - 1);
    float real_mesh_height = image.size().height / (float)(mesh_row_count - 1);

    for (size_t r = 0; r < mesh_row_count; ++r) {
      for (size_t c = 0; c < mesh_column_count; ++c) {
        target_graph.vertices_.push_back(glm::vec2(c * real_mesh_width, r * real_mesh_height));
      }
    }

    target_mesh = GLMesh();

    target_mesh.vertices_type = GL_QUADS;

    for (size_t r = 0; r < mesh_row_count - 1; ++r) {
      for (size_t c = 0; c < mesh_column_count - 1; ++c) {
        std::vector<size_t> vertex_indices;

        size_t base_index = r * mesh_column_count + c;
        vertex_indices.push_back(base_index);
        vertex_indices.push_back(base_index + mesh_column_count);
        vertex_indices.push_back(base_index + mesh_column_count + 1);
        vertex_indices.push_back(base_index + 1);

        if (!c) {
          target_graph.edges_.push_back(Edge(std::make_pair(vertex_indices[0], vertex_indices[1])));
        }

        target_graph.edges_.push_back(Edge(std::make_pair(vertex_indices[1], vertex_indices[2])));
        target_graph.edges_.push_back(Edge(std::make_pair(vertex_indices[3], vertex_indices[2])));

        if (!r) {
          target_graph.edges_.push_back(Edge(std::make_pair(vertex_indices[0], vertex_indices[3])));
        }

        for (const size_t vertex_index : vertex_indices) {
          target_mesh.vertices_.push_back(glm::vec3(target_graph.vertices_[vertex_index].x, target_graph.vertices_[vertex_index].y, 0.0f));
          target_mesh.uvs_.push_back(glm::vec2(target_graph.vertices_[vertex_index].x / (float)image.size().width, target_graph.vertices_[vertex_index].y / (float)image.size().height));
        }
      }
    }
  }

  void GLForm::BuildTriangleMesh(const cv::Mat &image, GLMesh &target_mesh, const float triangle_size) {

    triangulateio in, out;

    memset(&in, 0, sizeof(triangulateio));
    memset(&out, 0, sizeof(triangulateio));

    in.numberofpoints = 4;
    in.pointlist = (REAL *)malloc(in.numberofpoints * 2 * sizeof(REAL));

    in.pointlist[0] = 0;
    in.pointlist[1] = 0;

    in.pointlist[2] = 0;
    in.pointlist[3] = image.rows;

    in.pointlist[4] = image.cols;
    in.pointlist[5] = image.rows;

    in.pointlist[6] = image.cols;
    in.pointlist[7] = 0;

    char triangulate_instruction[100] = "";
    //sprintf(triangulate_instruction, "pqzQ");
    //sprintf(triangulate_instruction, "qa1000zQ");

    sprintf(triangulate_instruction, "qa%dzQ", (int)triangle_size);
    triangulate(triangulate_instruction, &in, &out, NULL);

    target_mesh = GLMesh();
    target_mesh.vertices_type = GL_TRIANGLES;

    for (int i = 0; i < out.numberoftriangles; ++i) {
      for (int j = 0; j < out.numberofcorners; ++j) {
        int target_vertex_index = out.trianglelist[i * out.numberofcorners + j];
        glm::vec3 vertex(out.pointlist[target_vertex_index * 2], out.pointlist[target_vertex_index * 2 + 1], 0);
        target_mesh.vertices_.push_back(vertex);
        glm::vec2 uv(vertex.x / (double)image.size().width, vertex.y / (double)image.size().height);
        target_mesh.uvs_.push_back(uv);
      }
    }

    free(in.pointlist);
    free(out.pointlist);
    free(out.pointattributelist);
    free(out.trianglelist);
    free(out.triangleattributelist);
  }

  void GLForm::GenerateDataForImageWarping() {
    if (!data_for_image_warping_were_generated) {
      std::cout << "Start : Gaussian smoothing\n";
      const double SMOOTH_SIGMA = 0.8;
      const cv::Size K_SIZE(3, 3);
      cv::GaussianBlur(source_image, source_image_after_smoothing, K_SIZE, SMOOTH_SIGMA);
      cv::imwrite(source_image_file_directory + "smooth_" + source_image_file_name, source_image_after_smoothing);
      std::cout << "Done : Gaussian smoothing\n";

      std::cout << "Start : Image segmentation\n";
      //const double SEGMENTATION_K = (source_image.size().width + source_image.size().height) / 1.75;
      const double SEGMENTATION_K = pow(source_image.size().width * source_image.size().height, 0.6);
      const double SEGMENTATION_MIN_PATCH_SIZE = (source_image.size().width * source_image.size().height) * 0.001;
      const double SEGMENTATION_SIMILAR_COLOR_MERGE_THRESHOLD = 20;

      source_image_after_segmentation = Segmentation(source_image, image_graph, group_of_pixel, SEGMENTATION_K, SEGMENTATION_MIN_PATCH_SIZE, SEGMENTATION_SIMILAR_COLOR_MERGE_THRESHOLD);
      cv::imwrite(source_image_file_directory + "segmentation_" + source_image_file_name + ".png", source_image_after_segmentation);
      std::cout << "Done : Image segmentation\n";

      std::cout << "Start : Image saliency calculation\n";
      const double SALIENCY_C = 3;
      const double SALIENCY_K = 64;
      saliency_map_of_source_image = CalculateContextAwareSaliencyMapWithMatlabProgram(source_image, saliency_map, source_image_file_directory + source_image_file_name, source_image_file_directory + "saliency_" + source_image_file_name + ".png");

      // Calculate the saliency value of each patch
      for (int r = 0; r < source_image.rows; ++r) {
        saliency_map[r].push_back(saliency_map[r].back());
        group_of_pixel[r].push_back(group_of_pixel[r].back());
      }
      saliency_map.push_back(saliency_map.back());
      group_of_pixel.push_back(group_of_pixel.back());

      int group_size = 0;
      for (int r = 0; r < (source_image.rows + 1); ++r) {
        for (int c = 0; c < (source_image.cols + 1); ++c) {
          group_size = std::max(group_size, group_of_pixel[r][c]);
        }
      }
      ++group_size;

      saliency_of_patch = std::vector<double>(group_size);
      std::vector<int> group_count(group_size);
      for (int r = 0; r < (source_image.rows + 1); ++r) {
        for (int c = 0; c < (source_image.cols + 1); ++c) {
          ++group_count[group_of_pixel[r][c]];
          saliency_of_patch[group_of_pixel[r][c]] += saliency_map[r][c];
        }
      }

      double min_saliency = 2e9, max_saliency = -2e9;
      for (int patch_index = 0; patch_index < group_size; ++patch_index) {
        if (group_count[patch_index]) {
          saliency_of_patch[patch_index] /= (double)group_count[patch_index];
        }
        min_saliency = std::min(min_saliency, saliency_of_patch[patch_index]);
        max_saliency = std::max(max_saliency, saliency_of_patch[patch_index]);
      }

      // Normalize saliency values
      for (int patch_index = 0; patch_index < group_size; ++patch_index) {
        saliency_of_patch[patch_index] = (saliency_of_patch[patch_index] - min_saliency) / (max_saliency - min_saliency);
      }

      significance_map_of_source_image = cv::Mat(source_image.size(), source_image.type());
      for (int r = 0; r < source_image.rows; ++r) {
        for (int c = 0; c < source_image.cols; ++c) {
          double vertex_saliency = saliency_of_patch[group_of_pixel[r][c]];
          significance_map_of_source_image.at<cv::Vec3b>(r, c) = SaliencyValueToSignifanceColor(vertex_saliency);
        }
      }
      cv::imwrite(source_image_file_directory + "significance_" + source_image_file_name + ".png", significance_map_of_source_image);

      std::cout << "Done : Image saliency calculation\n";

      data_for_image_warping_were_generated = true;
    }
  }

  void GLForm::ContentAwareImageRetargeting(const int target_image_width, const int target_image_height, const float grid_size) {
    if (!source_image.size().width || !source_image.size().height) {
      return;
    }

    GenerateDataForImageWarping();

    std::cout << "Start : Build mesh and graph\n";
    BuildGridMeshAndGraphForImage(source_image, gl_panel_image_mesh, image_graph, grid_size);
    std::cout << "Done : Build mesh and graph\n";

    //if (program_mode == PATCH_BASED_WARPING) {

    std::cout << "Start : Patch based warping\n";
    PatchBasedWarping(source_image, image_graph, group_of_pixel, saliency_of_patch, target_image_width, target_image_height, grid_size, grid_size);
    std::cout << "Done : Patch based warping\n";

    std::cout << "New image size : " << target_image_width << " " << target_image_height << "\n";
    //} else if (program_mode == FOCUS_WARPING) {
    //  std::cout << "Start : Focus warping\n";
    //  FocusWarping(image, image_graph, group_of_pixel, saliency_of_patch, target_image_width, target_image_height, mesh_width, mesh_height, focus_mesh_scale, focus_x, focus_y);
    //  std::cout << "Done : Focus warping\n";
    //  std::cout << "New image size : " << target_image_width << " " << target_image_height << "\n";
    //}

    saliency_of_mesh_vertex.clear();
    saliency_of_mesh_vertex = std::vector<double>(image_graph.vertices_.size());

    for (size_t vertex_index = 0; vertex_index < image_graph.vertices_.size(); ++vertex_index) {
      float original_x = gl_panel_image_mesh.vertices_[vertex_index].x;
      float original_y = gl_panel_image_mesh.vertices_[vertex_index].y;
      saliency_of_mesh_vertex[vertex_index] = saliency_of_patch[group_of_pixel[original_y][original_x]];
    }

    int mesh_column_count = (int)(source_image.size().width / grid_size) + 1;
    int mesh_row_count = (int)(source_image.size().height / grid_size) + 1;

    float real_mesh_width = source_image.size().width / (float)(mesh_column_count - 1);
    float real_mesh_height = source_image.size().height / (float)(mesh_row_count - 1);

    gl_panel_image_mesh.vertices_.clear();

    for (int r = 0; r < mesh_row_count - 1; ++r) {
      for (int c = 0; c < mesh_column_count - 1; ++c) {
        std::vector<size_t> vertex_indices;

        size_t base_index = r * (mesh_column_count)+c;
        vertex_indices.push_back(base_index);
        vertex_indices.push_back(base_index + mesh_column_count);
        vertex_indices.push_back(base_index + mesh_column_count + 1);
        vertex_indices.push_back(base_index + 1);

        for (const auto &vertex_index : vertex_indices) {
          gl_panel_image_mesh.vertices_.push_back(glm::vec3(image_graph.vertices_[vertex_index].x, image_graph.vertices_[vertex_index].y, 0));
        }
      }
    }

    gl_panel_image_mesh.colors_ = std::vector<glm::vec3>(gl_panel_image_mesh.vertices_.size(), glm::vec3(0, 0, 1));

    GLTexture::SetGLTexture(source_image, &gl_panel_image_mesh.texture_id_);

    gl_panel_image_mesh.Upload();

    //double cotanget_of_half_of_fovy = 1.0 / tan(glm::radians(FOVY / 2.0f));
    //eye_position.z = cotanget_of_half_of_fovy * (target_image_height / 2.0);

    RenderGLPanel();
  }

  void GLForm::ContentAwareVideoRetargetingUsingObjectPreservingWarping(const int target_video_width, const int target_video_height, const float grid_size) {
    if (!source_video_frames.size() || !source_video_frames[0].size().height || !source_video_frames[0].size().width) {
      return;
    }

    const std::string linear_video_path = source_video_file_directory + "linear_" + source_video_file_name + ".avi";
    cv::VideoWriter linear_video_writer;
    linear_video_writer.open(linear_video_path, /*CV_FOURCC('M', 'J', 'P', 'G')*/0, source_video_fps, cv::Size(gl_panel_->Width, gl_panel_->Height));

    for (const cv::Mat &frame : source_video_frames) {
      cv::Mat resize_frame;
      cv::resize(frame, resize_frame, cv::Size(gl_panel_->Width, gl_panel_->Height));
      linear_video_writer.write(resize_frame);
    }

    std::cout << "Done : Linear video\n";

    const std::string segmentation_video_path = source_video_file_directory + "segmentation_" + source_video_file_name + ".avi";
    const std::string significance_video_path = source_video_file_directory + "significance_" + source_video_file_name + ".avi";

    if (!std::fstream(significance_video_path).good() || !std::fstream(segmentation_video_path).good()) {
      std::cout << "Significance data for video warping not found.\n Generating new one.\n";
      std::map<size_t, size_t> object_sizes;
      saliency_of_object.clear();
      source_video_frames_after_segmentation.clear();

      if (!std::fstream(segmentation_video_path).good()) {
        VideoSegmentation(source_video_file_directory, source_video_file_name);
      }

      cv::VideoWriter segmentation_video_writer;
      segmentation_video_writer.open(segmentation_video_path, /*CV_FOURCC('M', 'J', 'P', 'G')*/0, source_video_fps, source_video_frames[0].size());

      cv::VideoWriter saliency_video_writer;
      saliency_video_writer.open(source_video_file_directory + "saliency_" + source_video_file_name + ".avi", /*CV_FOURCC('M', 'J', 'P', 'G')*/0, source_video_fps, source_video_frames[0].size());

      for (size_t t = 0; t < source_video_frames.size(); ++t) {
        std::ostringstream video_frame_name_oss;
        video_frame_name_oss << "frame" << std::setw(5) << std::setfill('0') << t << "_" + source_video_file_name + ".png";

        if (!std::fstream(source_video_file_directory + video_frame_name_oss.str()).good()) {
          cv::imwrite(source_video_file_directory + video_frame_name_oss.str(), source_video_frames[t]);
        }

        std::ostringstream video_frame_saliency_name_oss;
        video_frame_saliency_name_oss << "saliency_frame" << std::setw(5) << std::setfill('0') << t << "_" + source_video_file_name + ".png";

        std::vector<std::vector<double> > frame_saliency_map;
        cv::Mat saliency_frame = CalculateContextAwareSaliencyMapWithMatlabProgram(source_video_frames[t], frame_saliency_map, source_video_file_directory + video_frame_name_oss.str(), source_video_file_directory + video_frame_saliency_name_oss.str());

        std::ostringstream video_frame_segmentation_name_oss;
        video_frame_segmentation_name_oss << "segmentation_frame" << std::setw(5) << std::setfill('0') << t << "_" + source_video_file_name + ".png";

        if (std::fstream(source_video_file_directory + video_frame_segmentation_name_oss.str()).good()) {
          cv::Mat segmented_frame = cv::imread(source_video_file_directory + video_frame_segmentation_name_oss.str());
          //source_video_frames_after_segmentation.push_back(segmented_frame.clone());
          for (size_t r = 0; r < segmented_frame.size().height; ++r) {
            for (size_t c = 0; c < segmented_frame.size().width; ++c) {
              size_t pixel_group = Vec3bToValue(segmented_frame.at<cv::Vec3b>(r, c));
              ++object_sizes[pixel_group];
              saliency_of_object[pixel_group] += frame_saliency_map[r][c];
            }
          }

          segmentation_video_writer.write(segmented_frame);
          saliency_video_writer.write(saliency_frame);
        }
      }

      //segmentation_video_writer.release();
      //saliency_video_writer.release();

      std::cout << "Segmented frames and saliency frames are loaded.\n";

      //std::vector<std::vector<std::vector<int> > > group_of_pixel;
      //size_t real_group_count = CalculateGroupFromSegmentedVideo(source_video_frames_after_segmentation, group_of_pixel);
      //std::cout << "Real Group count : " << real_group_count << "\n";
      std::cout << "Group count : " << saliency_of_object.size() << "\n";

      // Calculate the saliency value of each object

      double min_saliency = 2e9, max_saliency = -2e9;
      for (const auto &object_size : object_sizes) {
        size_t pixel_group = object_size.first;
        if (object_sizes[pixel_group]) {
          saliency_of_object[pixel_group] /= object_sizes[pixel_group];
          min_saliency = std::min(min_saliency, saliency_of_object[pixel_group]);
          max_saliency = std::max(max_saliency, saliency_of_object[pixel_group]);
        }
      }

      std::map<size_t, cv::Vec3b> object_color;

      // Normalize saliency values
      for (const auto &object_size : object_sizes) {
        size_t pixel_group = object_size.first;
        if (object_sizes[pixel_group]) {
          saliency_of_object[pixel_group] = (saliency_of_object[pixel_group] - min_saliency) / (max_saliency - min_saliency);
        }
        object_color[pixel_group].val[0] = rand() % 256;
        object_color[pixel_group].val[1] = rand() % 256;
        object_color[pixel_group].val[2] = rand() % 256;
      }

      //std::vector<size_t> real_object_size(real_group_count, 0);
      //std::vector<double> real_saliency_of_object(real_group_count, 0);
      //std::vector<cv::Vec3b> real_object_color(real_group_count);

      //for (auto &color : real_object_color) {
      //  color.val[0] = rand() % 256;
      //  color.val[1] = rand() % 256;
      //  color.val[2] = rand() % 256;
      //}

      //min_saliency = 2e9;
      //max_saliency = -2e9;
      //for (size_t t = 0; t < source_video_frames_after_segmentation.size(); ++t) {
      //  std::ostringstream video_frame_saliency_name_oss;
      //  video_frame_saliency_name_oss << "saliency_frame" << std::setw(5) << std::setfill('0') << t << "_" + source_video_file_name + ".png";
      //  cv::Mat saliency_frame = cv::imread(source_video_file_directory + video_frame_saliency_name_oss.str());
      //  for (size_t r = 0; r < source_video_frames_after_segmentation[t].size().height; ++r) {
      //    for (size_t c = 0; c < source_video_frames_after_segmentation[t].size().width; ++c) {
      //      ++real_object_size[group_of_pixel[t][r][c]];
      //      real_saliency_of_object[group_of_pixel[t][r][c]] += saliency_frame.at<cv::Vec3b>(r, c).val[0] / 255.0;
      //    }
      //  }
      //}

      //for (size_t t = 0; t < real_object_size.size(); ++t) {
      //  if (real_object_size[t]) {
      //    real_saliency_of_object[t] /= (double)real_object_size[t];
      //  }
      //  min_saliency = std::min(min_saliency, real_saliency_of_object[t]);
      //  max_saliency = std::max(max_saliency, real_saliency_of_object[t]);
      //}

      cv::VideoWriter significance_video_writer;
      significance_video_writer.open(significance_video_path, /*CV_FOURCC('M', 'J', 'P', 'G')*/0, source_video_fps, source_video_frames[0].size());

      for (size_t t = 0; t < source_video_frames.size(); ++t) {

        std::ostringstream video_frame_segmentation_name_oss;
        video_frame_segmentation_name_oss << "segmentation_frame" << std::setw(5) << std::setfill('0') << t << "_" + source_video_file_name + ".png";

        if (!std::fstream(source_video_file_directory + video_frame_segmentation_name_oss.str()).good()) {
          break;
        }

        cv::Mat segmentation_video_frame = cv::imread(source_video_file_directory + video_frame_segmentation_name_oss.str());

        cv::Mat significance_frame = segmentation_video_frame.clone();
        for (size_t r = 0; r < significance_frame.size().height; ++r) {
          for (size_t c = 0; c < significance_frame.size().width; ++c) {
            size_t pixel_group = Vec3bToValue(significance_frame.at<cv::Vec3b>(r, c));
            double pixel_saliency = saliency_of_object[pixel_group];
            significance_frame.at<cv::Vec3b>(r, c) = SaliencyValueToSignifanceColor(pixel_saliency);
            //significance_frame.at<cv::Vec3b>(r, c) = object_color[pixel_group];

            //size_t pixel_group = group_of_pixel[t][r][c];
            //double pixel_saliency = real_saliency_of_object[pixel_group];
            //significance_frame.at<cv::Vec3b>(r, c) = SaliencyValueToSignifanceColor(pixel_saliency);
            ////significance_frame.at<cv::Vec3b>(r, c) = real_object_color[pixel_group];
          }
        }

        std::ostringstream video_frame_significance_name_oss;
        video_frame_significance_name_oss << "significance_frame" << std::setw(5) << std::setfill('0') << t << "_" + source_video_file_name + ".png";
        cv::imwrite(source_video_file_directory + video_frame_significance_name_oss.str(), significance_frame);

        significance_video_writer.write(significance_frame);
      }

      std::cout << "Done : Significance data for video warping\n";
    } else {
      std::cout << "Significance data for video warping has been found.\n";
    }

    cv::VideoCapture segmentation_video_capture;
    segmentation_video_capture.open(segmentation_video_path);
    size_t frame_count = segmentation_video_capture.get(CV_CAP_PROP_FRAME_COUNT);

    //std::vector<GLMesh> frames_mesh(frame_count);
    std::vector<Graph<glm::vec2> > frames_graph(frame_count);

    for (size_t t = 0; t < frame_count; ++t) {
      BuildGridMeshAndGraphForImage(source_video_frames[t], gl_panel_image_mesh, frames_graph[t], grid_size);
    }

    ObjectPreservingVideoWarping(source_video_file_directory, source_video_file_name, frames_graph, target_video_width, target_video_height, grid_size, grid_size);

    ChangeGLPanelSize(target_video_width, target_video_height);

    const std::string result_video_path = source_video_file_directory + "result_" + source_video_file_name + ".avi";
    cv::VideoWriter result_video_writer;
    result_video_writer.open(result_video_path, /*CV_FOURCC('M', 'J', 'P', 'G')*/0, source_video_fps, cv::Size(target_video_width, target_video_height));

    for (size_t t = 0; t < frame_count; ++t) {
      size_t mesh_column_count = (size_t)(source_video_frames[t].size().width / grid_size) + 1;
      size_t mesh_row_count = (size_t)(source_video_frames[t].size().height / grid_size) + 1;

      float real_mesh_width = source_video_frames[t].size().width / (float)(mesh_column_count - 1);
      float real_mesh_height = source_video_frames[t].size().height / (float)(mesh_row_count - 1);

      gl_panel_image_mesh.vertices_.clear();

      for (size_t r = 0; r < mesh_row_count - 1; ++r) {
        for (size_t c = 0; c < mesh_column_count - 1; ++c) {
          std::vector<size_t> vertex_indices;

          size_t base_index = r * (mesh_column_count)+c;
          vertex_indices.push_back(base_index);
          vertex_indices.push_back(base_index + mesh_column_count);
          vertex_indices.push_back(base_index + mesh_column_count + 1);
          vertex_indices.push_back(base_index + 1);

          for (const auto &vertex_index : vertex_indices) {
            gl_panel_image_mesh.vertices_.push_back(glm::vec3(frames_graph[t].vertices_[vertex_index].x, frames_graph[t].vertices_[vertex_index].y, 0));
          }
        }
      }

      gl_panel_image_mesh.colors_ = std::vector<glm::vec3>(gl_panel_image_mesh.vertices_.size(), glm::vec3(0, 0, 255));

      GLTexture::SetGLTexture(source_video_frames[t], &gl_panel_image_mesh.texture_id_);
      gl_panel_image_mesh.Upload();

      RenderGLPanel();

      std::ostringstream result_frame_name_oss;
      result_frame_name_oss << "result_frame" << std::setw(5) << std::setfill('0') << t << "_" + source_video_file_name + ".png";
      SaveGLScreen(source_video_file_directory + result_frame_name_oss.str());

      SaveGLScreenToVideo(result_video_writer);
    }
  }

  void GLForm::ImageFocusWarping(const int target_image_width, const int target_image_height, const float grid_size) {
    if (!source_image.size().width || !source_image.size().height) {
      return;
    }

    GenerateDataForImageWarping();

    std::cout << "Start : Build mesh and graph\n";
    BuildGridMeshAndGraphForImage(source_image, gl_panel_image_mesh, image_graph, grid_size);
    std::cout << "Done : Build mesh and graph\n";

    //if (program_mode == PATCH_BASED_WARPING) {

    double focus_grid_scale = focus_scale_track_bar_->Value * focus_grid_scale_modifier;
    std::cout << "Start : Focus warping\n";
    FocusWarping(source_image, image_graph, group_of_pixel, saliency_of_patch, target_image_width, target_image_height, grid_size, grid_size, focus_grid_scale, focus_position_x, focus_position_y);
    std::cout << "Done : Focus warping\n";

    std::cout << "New image size : " << target_image_width << " " << target_image_height << "\n";
    //} else if (program_mode == FOCUS_WARPING) {
    //  std::cout << "Start : Focus warping\n";
    //  FocusWarping(image, image_graph, group_of_pixel, saliency_of_patch, target_image_width, target_image_height, mesh_width, mesh_height, focus_mesh_scale, focus_x, focus_y);
    //  std::cout << "Done : Focus warping\n";
    //  std::cout << "New image size : " << target_image_width << " " << target_image_height << "\n";
    //}

    saliency_of_mesh_vertex.clear();
    saliency_of_mesh_vertex = std::vector<double>(image_graph.vertices_.size());

    for (size_t vertex_index = 0; vertex_index < image_graph.vertices_.size(); ++vertex_index) {
      float original_x = gl_panel_image_mesh.vertices_[vertex_index].x;
      float original_y = gl_panel_image_mesh.vertices_[vertex_index].y;
      saliency_of_mesh_vertex[vertex_index] = saliency_of_patch[group_of_pixel[original_y][original_x]];
    }

    int mesh_column_count = (int)(source_image.size().width / grid_size) + 1;
    int mesh_row_count = (int)(source_image.size().height / grid_size) + 1;

    float real_mesh_width = source_image.size().width / (float)(mesh_column_count - 1);
    float real_mesh_height = source_image.size().height / (float)(mesh_row_count - 1);

    gl_panel_image_mesh.vertices_.clear();

    for (int r = 0; r < mesh_row_count - 1; ++r) {
      for (int c = 0; c < mesh_column_count - 1; ++c) {
        std::vector<size_t> vertex_indices;

        size_t base_index = r * (mesh_column_count)+c;
        vertex_indices.push_back(base_index);
        vertex_indices.push_back(base_index + mesh_column_count);
        vertex_indices.push_back(base_index + mesh_column_count + 1);
        vertex_indices.push_back(base_index + 1);

        for (const auto &vertex_index : vertex_indices) {
          gl_panel_image_mesh.vertices_.push_back(glm::vec3(image_graph.vertices_[vertex_index].x, image_graph.vertices_[vertex_index].y, 0));
        }
      }
    }

    gl_panel_image_mesh.colors_ = std::vector<glm::vec3>(gl_panel_image_mesh.vertices_.size(), glm::vec3(0, 0, 255));

    GLTexture::SetGLTexture(source_image, &gl_panel_image_mesh.texture_id_);

    gl_panel_image_mesh.Upload();

    //double cotanget_of_half_of_fovy = 1.0 / tan(glm::radians(FOVY / 2.0f));
    //eye_position.z = cotanget_of_half_of_fovy * (target_image_height / 2.0);

    RenderGLPanel();
  }

  cv::Vec3b GLForm::SaliencyValueToSignifanceColor(double saliency_value) {
    cv::Vec3b signifance_color(0, 0, 0);

    if (saliency_value > 1) {
      signifance_color[2] = 1;
    }

    if (saliency_value < 0) {
      signifance_color[0] = 1;
    }

    if (saliency_value < (1 / 3.0)) {
      signifance_color[1] = (saliency_value * 3.0) * 255;
      signifance_color[0] = (1 - saliency_value * 3.0) * 255;
    } else if (saliency_value < (2 / 3.0)) {
      signifance_color[2] = ((saliency_value - (1 / 3.0)) * 3.0) * 255;
      signifance_color[1] = 1.0 * 255;
    } else if (saliency_value <= 1) {
      signifance_color[2] = 1.0 * 255;
      signifance_color[1] = (1.0 - (saliency_value - (2 / 3.0)) * 3.0) * 255;
    }

    return signifance_color;
  }

  size_t GLForm::Vec3bToValue(const cv::Vec3b &color) {
    return (size_t)color.val[0] * 256 * 256 + (size_t)color.val[1] * 256 + (size_t)color.val[2];
  }

  void GLForm::OnButtonsClick(System::Object ^sender, System::EventArgs ^e) {
    if (sender == open_image_file_tool_strip_menu_item_) {
      OpenFileDialog ^open_image_file_dialog = gcnew OpenFileDialog();
      open_image_file_dialog->Filter = "image files|*.*";
      open_image_file_dialog->Title = "Open an image file.";

      if (open_image_file_dialog->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
        std::string raw_file_path = msclr::interop::marshal_as<std::string>(open_image_file_dialog->FileName);
        source_image_file_directory = msclr::interop::marshal_as<std::string>(System::IO::Path::GetDirectoryName(open_image_file_dialog->FileName)) + "\\";
        source_image_file_name = msclr::interop::marshal_as<std::string>(open_image_file_dialog->SafeFileName);

        source_image = cv::imread(raw_file_path);

        ChangeGLPanelSize(source_image.size().width, source_image.size().height);

        original_size_label_->Text = "Original size : " + gl_panel_->Width + " X " + gl_panel_->Height;

        BuildGridMeshAndGraphForImage(source_image, gl_panel_image_mesh, image_graph, current_grid_size);

        gl_panel_image_mesh.colors_ = std::vector<glm::vec3>(gl_panel_image_mesh.vertices_.size(), glm::vec3(0, 0, 255));

        GLTexture::SetGLTexture(source_image, &gl_panel_image_mesh.texture_id_);

        gl_panel_image_mesh.Upload();

        RenderGLPanel();

        data_for_image_warping_were_generated = false;

        //is_demo_triangle = true;

        ChangeProgramStatus(ProgramStatus::IMAGE_RETARGETING);
        //ChangeProgramStatus(ProgramStatus::IMAGE_FOCUS);
      }

    } else if (sender == open_video_file_tool_strip_menu_item_) {
      OpenFileDialog ^open_image_file_dialog = gcnew OpenFileDialog();
      open_image_file_dialog->Filter = "video files|*.*";
      open_image_file_dialog->Title = "Open a video file.";

      if (open_image_file_dialog->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
        std::string raw_file_path = msclr::interop::marshal_as<std::string>(open_image_file_dialog->FileName);
        source_video_file_directory = msclr::interop::marshal_as<std::string>(System::IO::Path::GetDirectoryName(open_image_file_dialog->FileName)) + "\\";
        source_video_file_name = msclr::interop::marshal_as<std::string>(open_image_file_dialog->SafeFileName);

        cv::VideoCapture video_capture;

        video_capture.open(raw_file_path);

        source_video_fps = video_capture.get(CV_CAP_PROP_FPS);
        source_video_fourcc = video_capture.get(CV_CAP_PROP_FOURCC);

        source_video_frames.clear();

        cv::Mat video_frame;

        while (video_capture.read(video_frame)) {
          source_video_frames.push_back(video_frame.clone());
        }

        if (source_video_frames.size()) {

          ChangeGLPanelSize(source_video_frames[0].size().width * 0.5, source_video_frames[0].size().height);

          original_size_label_->Text = "Original size : " + source_video_frames[0].size().width + " X " + source_video_frames[0].size().height;

          BuildGridMeshAndGraphForImage(source_video_frames[0], gl_panel_image_mesh, image_graph, current_grid_size);

          gl_panel_image_mesh.colors_ = std::vector<glm::vec3>(gl_panel_image_mesh.vertices_.size(), glm::vec3(0, 0, 1));

          GLTexture::SetGLTexture(source_video_frames[0], &gl_panel_image_mesh.texture_id_);

          gl_panel_image_mesh.Upload();

          RenderGLPanel();
        }

        ChangeProgramStatus(ProgramStatus::VIDEO_RETARGETING);
      }

    } else if (sender == save_screen_tool_strip_menu_item_) {

      if (!gl_panel_->Width || !gl_panel_->Height) {
        std::cout << "GL panel size not correct (<= 0)\n";
        return;
      }

      SaveFileDialog ^save_screen_file_dialog = gcnew SaveFileDialog();
      save_screen_file_dialog->Filter = "jpg|*.jpg|bmp|*.bmp|png|*.png";
      save_screen_file_dialog->Title = "Save screen as a image file.";

      if (save_screen_file_dialog->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
        System::IO::Stream^ image_file_stream;
        if ((image_file_stream = save_screen_file_dialog->OpenFile()) != nullptr && save_screen_file_dialog->FileName != "") {
          image_file_stream->Close();

          std::string screen_file_path = msclr::interop::marshal_as<std::string>(save_screen_file_dialog->FileName);

          SaveGLScreen(screen_file_path);
        }
      }
    } else if (sender == start_button_) {

      if (program_status == ProgramStatus::IMAGE_RETARGETING) {
        ContentAwareImageRetargeting(gl_panel_->Width, gl_panel_->Height, current_grid_size);
      } else if (program_status == ProgramStatus::VIDEO_RETARGETING) {
        ContentAwareVideoRetargetingUsingObjectPreservingWarping(gl_panel_->Width, gl_panel_->Height, current_grid_size);
      }
    }
  }

  void GLForm::OnMouseDown(System::Object ^sender, System::Windows::Forms::MouseEventArgs ^e) {
    if (sender == gl_panel_) {
      is_dragging_panel = true;
    }
  }

  void GLForm::OnMouseUp(System::Object ^sender, System::Windows::Forms::MouseEventArgs ^e) {
    if (sender == gl_panel_) {
      if (is_dragging_panel) {
        is_dragging_panel = false;

        if (program_status == ProgramStatus::IMAGE_RETARGETING) {
          ContentAwareImageRetargeting(gl_panel_->Width, gl_panel_->Height, current_grid_size);
        }
      }
    }
  }

  void GLForm::OnMouseMove(System::Object ^sender, System::Windows::Forms::MouseEventArgs ^e) {
    if (sender == gl_panel_) {
      if (is_dragging_panel) {
        int new_panel_width = e->X;
        int new_panel_height = e->Y;
        ChangeGLPanelSize(new_panel_width, new_panel_height);

        //BuildGridMeshAndGraphForImage(source_image, gl_panel_image_mesh, image_graph, 10);

        //GLTexture::SetGLTexture(source_image, &gl_panel_image_mesh.texture_id_);

        //gl_panel_image_mesh.Upload();

        //BuildLinesMeshFromMesh(gl_panel_image_mesh, gl_panel_lines_mesh);
        //gl_panel_lines_mesh.Upload();

      }

      if (program_status == ProgramStatus::IMAGE_FOCUS) {
        double new_focus_position_x = e->X;
        double new_focus_position_y = gl_panel_->Height - e->Y;
        if (std::abs(new_focus_position_x - focus_position_x) >= 1 || std::abs(new_focus_position_y - focus_position_y) >= 1) {
          ImageFocusWarping(gl_panel_->Width, gl_panel_->Height, current_grid_size);
        }
        focus_position_x = new_focus_position_x;
        focus_position_y = new_focus_position_y;
      }

      if (is_demo_triangle) {
        BuildTriangleMesh(source_image, gl_panel_image_triangle_mesh, current_grid_size * 10);

        gl_panel_image_triangle_mesh.colors_ = std::vector<glm::vec3>(gl_panel_image_triangle_mesh.vertices_.size(), glm::vec3(0, 0, 1));

        GLTexture::SetGLTexture(source_image, &gl_panel_image_triangle_mesh.texture_id_);
        gl_panel_image_triangle_mesh.Upload();
      }

      RenderGLPanel();
    }
  }

  void GLForm::OnKeyDown(System::Object ^sender, System::Windows::Forms::KeyEventArgs ^e) {

    switch (e->KeyCode) {
    case Keys::Add:
      --eye_position.z;
      break;
    case Keys::Subtract:
      ++eye_position.z;
      break;
    case Keys::Up:
      ++eye_position.y;
      ++look_at_position.y;
      break;
    case Keys::Down:
      --eye_position.y;
      --look_at_position.y;
      break;
    case Keys::Left:
      --eye_position.x;
      --look_at_position.x;
      break;
    case Keys::Right:
      ++eye_position.x;
      ++look_at_position.x;
      break;
    case Keys::W:
      ChangeGLPanelSize(gl_panel_->Width, gl_panel_->Height - 1);
      break;
    case Keys::A:
      ChangeGLPanelSize(gl_panel_->Width - 1, gl_panel_->Height);
      break;
    case Keys::S:
      ChangeGLPanelSize(gl_panel_->Width, gl_panel_->Height + 1);
      break;
    case Keys::D:
      ChangeGLPanelSize(gl_panel_->Width + 1, gl_panel_->Height);
      break;
    case Keys::L:
      // Checking memory status
      for (glm::vec3 &color : gl_panel_image_mesh.colors_) {
        color = glm::vec3(rand() / (double)RAND_MAX, rand() / (double)RAND_MAX, rand() / (double)RAND_MAX);
      }
      gl_panel_image_mesh.Upload();
      // Checking memory status
      show_lines_check_box_->Checked = !show_lines_check_box_->Checked;
      break;
    case Keys::T:
      if (!source_image.empty()) {
        is_demo_triangle = !is_demo_triangle;
      }
      break;
    }

    RenderGLPanel();
  }

  void GLForm::OnTrackBarsValueChanged(System::Object ^sender, System::EventArgs ^e) {
    if (sender == grid_size_track_bar_) {
      current_grid_size = grid_size_track_bar_->Value;
      if (program_status == ProgramStatus::IMAGE_FOCUS) {
        current_grid_size = std::max(MIN_GRID_SIZE_WHEN_FOCUS, current_grid_size);
        grid_size_track_bar_->Value = current_grid_size;
      }
      grid_size_label_->Text = "Grid size : " + current_grid_size;
    } else if (sender == focus_scale_track_bar_) {
      double focus_scale = focus_scale_track_bar_->Value * focus_grid_scale_modifier;
      focus_scale_label_->Text = "Focus scale : " + focus_scale;
    }
    RenderGLPanel();
  }

  void GLForm::OnCheckBoxesCheckedChanged(System::Object ^sender, System::EventArgs ^e) {
    if (sender == focus_check_box_) {
      if (focus_check_box_->Checked) {
        program_status = ProgramStatus::IMAGE_FOCUS;
        current_grid_size = std::max(MIN_GRID_SIZE_WHEN_FOCUS, current_grid_size);
        grid_size_track_bar_->Value = current_grid_size;
      } else if (program_status == ProgramStatus::IMAGE_FOCUS) {
        program_status = ProgramStatus::IMAGE_RETARGETING;
      }
    }
    RenderGLPanel();
  }

  void GLForm::OnNumericUpDownValueChanged(System::Object ^sender, System::EventArgs ^e) {
    if (sender == target_height_numeric_up_down_ || sender == target_width_numeric_up_down_) {
      ChangeGLPanelSize(System::Decimal::ToInt32(target_width_numeric_up_down_->Value), System::Decimal::ToInt32(target_height_numeric_up_down_->Value));
    }
    RenderGLPanel();
  }

}
